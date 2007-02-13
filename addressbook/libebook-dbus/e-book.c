/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Copyright (C) 2006 OpenedHand Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU Lesser General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Author: Ross Burton <ross@openedhand.com>
 */

#include <config.h>
#include <unistd.h>
#include <string.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <dbus/dbus-glib.h>
#include "e-book.h"
#include "e-error.h"
#include "e-contact.h"
#include "e-book-view-private.h"
#include "e-data-book-factory-bindings.h"
#include "e-data-book-bindings.h"
#include "libedata-book/e-data-book-types.h"
#include "e-book-marshal.h"

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib-lowlevel.h>

#define E_DATA_BOOK_FACTORY_SERVICE_NAME "org.gnome.evolution.dataserver.AddressBook"

static char** flatten_stringlist(GList *list);
static GList *array_to_stringlist (char **list);
static gboolean unwrap_gerror(GError *error, GError **client_error);
static EBookStatus get_status_from_error (GError *error);

G_DEFINE_TYPE(EBook, e_book, G_TYPE_OBJECT)
#define E_BOOK_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), E_TYPE_BOOK, EBookPrivate))

enum {
	WRITABLE_STATUS,
	CONNECTION_STATUS,
	AUTH_REQUIRED,
	BACKEND_DIED,
	LAST_SIGNAL
};

static guint e_book_signals [LAST_SIGNAL];

struct _EBookPrivate {
  ESource *source;
  char *uri;
  DBusGProxy *proxy;
  gboolean loaded;
  gboolean writable;
  gboolean connected;
  char *cap;
  gboolean cap_queried;
};

static DBusGConnection *connection = NULL;
static DBusGProxy *factory_proxy = NULL;

struct async_data {
  EBook *book;
  void *callback; /* TODO union */
  gpointer closure;
  gpointer data;
};

GQuark
e_book_error_quark (void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string ("e-book-error-quark");
  return q;
}

/*
 * Called when the addressbook server dies.
 */
static void
proxy_destroyed (gpointer data, GObject *object)
{
  EBook *book = data;

  g_assert (E_IS_BOOK (book));

  g_warning ("e-d-s proxy died");

  /* Ensure that everything relevant is NULL */
  factory_proxy = NULL;
  book->priv->proxy = NULL;

  g_signal_emit (G_OBJECT (book), e_book_signals [BACKEND_DIED], 0);
}

static void
e_book_dispose (GObject *object)
{
  EBook *book = E_BOOK (object);

  book->priv->loaded = FALSE;

  if (book->priv->proxy) {
    g_object_weak_unref (G_OBJECT (book->priv->proxy), proxy_destroyed, book);
    org_gnome_evolution_dataserver_addressbook_Book_close (book->priv->proxy, NULL);
  }
  if (book->priv->source) {
    g_object_unref (book->priv->source);
    book->priv->source = NULL;
  }
  if (book->priv->proxy) {
    g_object_unref (book->priv->proxy);
    book->priv->proxy = NULL;
  }

  if (G_OBJECT_CLASS (e_book_parent_class)->dispose)
    G_OBJECT_CLASS (e_book_parent_class)->dispose (object);
}

static void
e_book_finalize (GObject *object)
{
  EBook *book = E_BOOK (object);

  if (book->priv->uri)
    g_free (book->priv->uri);

  if (book->priv->cap)
    g_free (book->priv->cap);

  if (G_OBJECT_CLASS (e_book_parent_class)->finalize)
    G_OBJECT_CLASS (e_book_parent_class)->finalize (object);
}

static void
e_book_class_init (EBookClass *e_book_class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (e_book_class);

  e_book_signals [WRITABLE_STATUS] =
		g_signal_new ("writable_status",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EBookClass, writable_status),
			      NULL, NULL,
			      e_book_marshal_NONE__BOOL,
			      G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);
  
  e_book_signals [CONNECTION_STATUS] =
    g_signal_new ("connection_status",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (EBookClass, connection_status),
                  NULL, NULL,
                  e_book_marshal_NONE__BOOL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);
  
  e_book_signals [AUTH_REQUIRED] =
    g_signal_new ("auth_required",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (EBookClass, auth_required),
                  NULL, NULL,
                  e_book_marshal_NONE__NONE,
                  G_TYPE_NONE, 0);

  e_book_signals [BACKEND_DIED] =
    g_signal_new ("backend_died",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (EBookClass, backend_died),
                  NULL, NULL,
                  e_book_marshal_NONE__NONE,
                  G_TYPE_NONE, 0);
  
  gobject_class->dispose = e_book_dispose;
  gobject_class->finalize = e_book_finalize;

  g_type_class_add_private (e_book_class, sizeof (EBookPrivate));
}

static void
e_book_init (EBook *book)
{
  EBookPrivate *priv = E_BOOK_GET_PRIVATE (book);
  priv->source = NULL;
  priv->uri = NULL;
  priv->proxy = NULL;
  priv->loaded = FALSE;
  priv->writable = FALSE;
  priv->connected = FALSE;
  priv->cap = NULL;
  priv->cap_queried = FALSE;
  book->priv = priv;
}

/* one-time start up for libebook */
static gboolean
e_book_activate(GError **error)
{
  DBusError derror;

  if (G_LIKELY (factory_proxy)) {
    return TRUE;
  }

  if (!connection) {
    connection = dbus_g_bus_get (DBUS_BUS_SESSION, error);
    if (!connection)
      return FALSE;
  }

  dbus_error_init (&derror);
  if (!dbus_bus_start_service_by_name (dbus_g_connection_get_connection (connection),
                                       E_DATA_BOOK_FACTORY_SERVICE_NAME,
                                       0, NULL, &derror)) {
    dbus_set_g_error (error, &derror);
    dbus_error_free (&derror);
    return FALSE;
  }

  if (!factory_proxy) {
    factory_proxy = dbus_g_proxy_new_for_name_owner (connection,
                                                     E_DATA_BOOK_FACTORY_SERVICE_NAME,
                                                     "/org/gnome/evolution/dataserver/addressbook/BookFactory",
                                                     "org.gnome.evolution.dataserver.addressbook.BookFactory",
                                                     error);
    if (!factory_proxy) {
      return FALSE;
    }
    g_object_add_weak_pointer (G_OBJECT (factory_proxy), (gpointer)&factory_proxy);
  }

  return TRUE;
}

static void
writable_cb (DBusGProxy *proxy, gboolean writable, EBook *book)
{
  g_return_if_fail (E_IS_BOOK (book));

  book->priv->writable = writable;

  g_signal_emit (G_OBJECT (book), e_book_signals [WRITABLE_STATUS], 0, writable);
}

static void
connection_cb (DBusGProxy *proxy, gboolean connected, EBook *book)
{
  g_return_if_fail (E_IS_BOOK (book));

  book->priv->connected = connected;

  g_signal_emit (G_OBJECT (book), e_book_signals [CONNECTION_STATUS], 0, connected);
}

static void
auth_required_cb (DBusGProxy *proxy, EBook *book)
{
  g_return_if_fail (E_IS_BOOK (book));

  g_signal_emit (G_OBJECT (book), e_book_signals [AUTH_REQUIRED], 0);
}

/**
 * e_book_get_addressbooks:
 * @addressbook_sources: A pointer to a ESourceList* to set
 * @error: A pointer to a GError* to set on error
 *
 * Populate *addressbook_sources with the list of all sources which have been
 * added to Evolution.
 *
 * Return value: %TRUE if @addressbook_sources was set, otherwise %FALSE.
 */
gboolean
e_book_get_addressbooks (ESourceList **addressbook_sources, GError **error)
{
	GConfClient *gconf;
	
	e_return_error_if_fail (addressbook_sources, E_BOOK_ERROR_INVALID_ARG);

	gconf = gconf_client_get_default();
	*addressbook_sources = e_source_list_new_for_gconf (gconf, "/apps/evolution/addressbook/sources");
	g_object_unref (gconf);

	return TRUE;
}

/**
 * e_book_new:
 *
 * @source: An #ESource pointer
 * @error: A #GError pointer
 *
 * Creates a new #EBook corresponding to the given source.  There are
 * only two operations that are valid on this book at this point:
 * e_book_open(), and e_book_remove().
 *
 * Return value: a new but unopened #EBook.
 */
EBook*
e_book_new (ESource *source, GError **error)
{
  GError *err = NULL;
  EBook *book;
  const char *address;
  char *path;
  
  e_return_error_if_fail (E_IS_SOURCE (source), E_BOOK_ERROR_INVALID_ARG);
  
  if (!e_book_activate (&err)) {
    g_warning ("Cannot activate book: %s\n", err->message);
    g_propagate_error (error, err);
    return NULL;
  }

  book = g_object_new (E_TYPE_BOOK, NULL);

  book->priv->source = g_object_ref (source);
  book->priv->uri = e_source_get_uri (source);
  
  address = dbus_bus_get_unique_name (dbus_g_connection_get_connection (connection));

  if (!org_gnome_evolution_dataserver_addressbook_BookFactory_get_book (factory_proxy, address, book->priv->uri, &path, &err)) {
    g_warning ("Cannot get book from factory: %s", err ? err->message : "[no error]");
    g_propagate_error (error, err);
    g_object_unref (book);
    return NULL;
  }
  
  book->priv->proxy = dbus_g_proxy_new_for_name_owner (connection,
                                                       E_DATA_BOOK_FACTORY_SERVICE_NAME, path,
                                                       "org.gnome.evolution.dataserver.addressbook.Book",
                                                       &err);
  if (!book->priv->proxy) {
    g_warning ("Cannot get proxy for book %s: %s", path, err->message);
    g_propagate_error (error, err);
    g_free (path);
    g_object_unref (book);
    return NULL;
  }
  g_free (path);

  g_object_weak_ref (G_OBJECT (book->priv->proxy), proxy_destroyed, book);

  dbus_g_proxy_add_signal (book->priv->proxy, "writable", G_TYPE_BOOLEAN, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (book->priv->proxy, "writable", G_CALLBACK (writable_cb), book, NULL);
  dbus_g_proxy_add_signal (book->priv->proxy, "connection", G_TYPE_BOOLEAN, G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (book->priv->proxy, "connection", G_CALLBACK (connection_cb), book, NULL);
  dbus_g_proxy_add_signal (book->priv->proxy, "auth_required", G_TYPE_INVALID);
  dbus_g_proxy_connect_signal (book->priv->proxy, "auth_required", G_CALLBACK (auth_required_cb), book, NULL);

  return book;
}

/**
 * e_book_new_from_uri:
 *
 * @uri: the URI to load
 * @error: A #GError pointer
 *
 * Creates a new #EBook corresponding to the given uri.  See the
 * documentation for e_book_new for further information.
 *
 * Return value: a new but unopened #EBook.
 */
EBook*
e_book_new_from_uri (const char *uri, GError **error)
{
  ESource *source;
  EBook *book;
  
  e_return_error_if_fail (uri, E_BOOK_ERROR_INVALID_ARG);
  
  source = e_source_new_with_absolute_uri ("", uri);
  
  book = e_book_new (source, error);
  
  g_object_unref (source);
  
  return book;
}

/**
 * e_book_new_system_addressbook:
 *
 * @uri: the URI to load
 * @error: A #GError pointer
 *
 * Creates a new #EBook corresponding to the user's system
 * addressbook.  See the documentation for e_book_new for further
 * information.
 *
 * Return value: a new but unopened #EBook.
 */
EBook*
e_book_new_system_addressbook (GError **error)
{
	ESourceList *sources;
	GSList *g;
	ESource *system_source = NULL;
	EBook *book = NULL;

	if (!e_book_get_addressbooks (&sources, error)) {
		return FALSE;
	}

	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);

			if (e_source_get_property (source, "system")) {
				system_source = source;
				break;
			}
		}

		if (system_source)
			break;
	}

	if (system_source) {
		book = e_book_new (system_source, error);
	}
	else {
		char *filename;
		char *uri;

		filename = g_build_filename (g_get_home_dir(),
					     ".evolution/addressbook/local/system",
					     NULL);
		uri = g_strdup_printf ("file://%s", filename);

		g_free (filename);
	
		book = e_book_new_from_uri (uri, error);

		g_free (uri);
	}

	g_object_unref (sources);

	return book;
}

/**
 * e_book_new_default_addressbook:
 *
 * @uri: the URI to load
 * @error: A #GError pointer
 *
 * Creates a new #EBook corresponding to the user's default
 * addressbook.  See the documentation for e_book_new for further
 * information.
 *
 * Return value: a new but unopened #EBook.
 */
EBook*
e_book_new_default_addressbook   (GError **error)
{
	ESourceList *sources;
	GSList *g;
	ESource *default_source = NULL;
	EBook *book;

	if (!e_book_get_addressbooks (&sources, error)) {
		return FALSE;
	}

	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		ESourceGroup *group = E_SOURCE_GROUP (g->data);
		GSList *s;
		for (s = e_source_group_peek_sources (group); s; s = s->next) {
			ESource *source = E_SOURCE (s->data);

			if (e_source_get_property (source, "default")) {
				default_source = source;
				break;
			}
		}

		if (default_source)
			break;
	}

	if (default_source)
		book = e_book_new (default_source, error);
	else
		book = e_book_new_system_addressbook (error);

	g_object_unref (sources);

	return book;
}

ESource*
e_book_get_source (EBook *book)
{
  g_return_val_if_fail (E_IS_BOOK (book), NULL);

  return book->priv->source;
}

gboolean
e_book_open (EBook *book, gboolean only_if_exists, GError **error)
{
  GError *err = NULL;
  EBookStatus status;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  
  if (!org_gnome_evolution_dataserver_addressbook_Book_open (book->priv->proxy, only_if_exists, &err)) {
    g_propagate_error (error, err);
    return FALSE;
  }

  status = get_status_from_error (err);
  
  if (status == E_BOOK_ERROR_OK) {
    book->priv->loaded = TRUE;
    return TRUE;
  } else {
    g_propagate_error (error, err);
    return FALSE;
  }
}

static void
open_reply(DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  EDataBookStatus status;

  status = get_status_from_error (error);

  data->book->priv->loaded = (status == E_BOOK_ERROR_OK);

  if (cb)
    cb (data->book, status, data->closure);
  g_free (data);
}

guint
e_book_async_open (EBook *book, gboolean only_if_exists, EBookCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_open_async (book->priv->proxy, only_if_exists, open_reply, data);
  return 0;
}

gboolean
e_book_remove (EBook *book, GError **error)
{
  GError *err = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  
  org_gnome_evolution_dataserver_addressbook_Book_remove (book->priv->proxy, &err);
  return unwrap_gerror (err, error);
}

static void
remove_reply(DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  if (cb)
    cb (data->book, get_status_from_error (error), data->closure);
  g_free (data);
}

guint
e_book_async_remove (EBook *book, EBookCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_remove_async (book->priv->proxy, remove_reply, data);
  return 0;
}

gboolean
e_book_get_required_fields (EBook *book, GList **fields, GError **error)
{
  GError *err = NULL;
  char **list = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  org_gnome_evolution_dataserver_addressbook_Book_get_required_fields (book->priv->proxy, &list, &err);
  if (list) {
    *fields = array_to_stringlist (list);
    return TRUE;
  } else {
    return unwrap_gerror (err, error);
  }
}

static void
get_required_fields_reply(DBusGProxy *proxy, char **fields, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookEListCallback cb = data->callback;
  char **i = fields;
  EList *efields = e_list_new (NULL, 
                               (EListFreeFunc) g_free,
                               NULL);
  
  while (*i != NULL) {
    e_list_append (efields, (*i++));
  }
  
  if (cb)
    cb (data->book, get_status_from_error (error), efields, data->closure);

  g_object_unref (efields);
  g_free (fields);
}

guint
e_book_async_get_required_fields (EBook *book, EBookEListCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_get_required_fields_async (book->priv->proxy, get_required_fields_reply, data);
  return 0;
}

gboolean
e_book_get_supported_fields (EBook *book, GList **fields, GError **error)
{
  GError *err = NULL;
  char **list = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  org_gnome_evolution_dataserver_addressbook_Book_get_supported_fields (book->priv->proxy, &list, &err);
  if (list) {
    *fields = array_to_stringlist (list);
    return TRUE;
  } else {
    return unwrap_gerror (err, error);
  }
}

static void
get_supported_fields_reply(DBusGProxy *proxy, char **fields, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookEListCallback cb = data->callback;
  char **i = fields;
  EList *efields = e_list_new (NULL,  (EListFreeFunc) g_free, NULL);
  
  while (*i != NULL) {
    e_list_append (efields, (*i++));
  }
  
  if (cb)
    cb (data->book, get_status_from_error (error), efields, data->closure);

  g_object_unref (efields);
  g_free (fields);
}

guint
e_book_async_get_supported_fields (EBook *book, EBookEListCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_get_supported_fields_async (book->priv->proxy, get_supported_fields_reply, data);
  return 0;
}

gboolean
e_book_get_supported_auth_methods (EBook *book, GList **auth_methods, GError **error)
{
  GError *err = NULL;
  char **list = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  org_gnome_evolution_dataserver_addressbook_Book_get_supported_auth_methods (book->priv->proxy, &list, &err);
  if (list) {
    *auth_methods = array_to_stringlist (list);
    return TRUE;
  } else {
    return unwrap_gerror (err, error);
  }
}

static void
get_supported_auth_methods_reply(DBusGProxy *proxy, char **methods, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookEListCallback cb = data->callback;
  char **i = methods;
  EList *emethods = e_list_new (NULL, 
                                (EListFreeFunc) g_free,
                                NULL);

  while (*i != NULL) {
    e_list_append (emethods, (*i++));
  }
  
  if (cb)
    cb (data->book, get_status_from_error (error), emethods, data->closure);

  g_object_unref (emethods);
  g_free (methods);
}

guint
e_book_async_get_supported_auth_methods (EBook *book, EBookEListCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_get_supported_auth_methods_async (book->priv->proxy, get_supported_auth_methods_reply, data);
  return 0;
}

gboolean
e_book_authenticate_user (EBook *book, const char *user, const char *passwd, const char *auth_method, GError **error)
{
  GError *err = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  org_gnome_evolution_dataserver_addressbook_Book_authenticate_user (book->priv->proxy, user, passwd, auth_method, &err);
  return unwrap_gerror (err, error);
}

static void
authenticate_user_reply(DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  if (cb)
    cb (data->book, get_status_from_error (error), data->closure);
  g_free (data);
}

guint
e_book_async_authenticate_user (EBook *book, const char *user, const char *passwd, const char *auth_method, EBookCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_if_fail (user, E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (passwd, E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (auth_method, E_BOOK_ERROR_INVALID_ARG);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_authenticate_user_async (book->priv->proxy, user, passwd, auth_method, authenticate_user_reply, data);
  return 0;
}

gboolean
e_book_get_contact (EBook *book, const char  *id, EContact **contact, GError **error)
{
  GError *err = NULL;
  char *vcard = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  
  org_gnome_evolution_dataserver_addressbook_Book_get_contact (book->priv->proxy, id, &vcard, &err);
  if (vcard) {
    *contact = e_contact_new_from_vcard (vcard);
    g_free (vcard);
  }
  return unwrap_gerror (err, error);
}

static void
get_contact_reply(DBusGProxy *proxy, char *vcard, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookContactCallback cb = data->callback;
  EBookStatus status = get_status_from_error (error);

  /* Protect against garbage return values on error */
  if (error)
	  vcard = NULL;

  if (cb && error == NULL) {
	  cb (data->book, status, e_contact_new_from_vcard (vcard), data->closure);
  } else if (cb && error) {
	  cb (data->book, status, NULL, data->closure);
  } else if (cb == NULL && error) {
	  g_warning ("Cannot get contact: %s", error->message);
  }

  if (error)
	  g_error_free (error);
  g_free (vcard);
  g_free (data);
}

guint
e_book_async_get_contact (EBook *book, const char *id, EBookContactCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_val_if_fail (id, E_BOOK_ERROR_INVALID_ARG);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_get_contact_async (book->priv->proxy, id, get_contact_reply, data);
  return 0;
}

gboolean
e_book_get_contacts (EBook *book, EBookQuery *query, GList **contacts, GError **error)
{
  GError *err = NULL;
  char **list = NULL;
  char *sexp;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  sexp = e_book_query_to_string (query);
  org_gnome_evolution_dataserver_addressbook_Book_get_contact_list (book->priv->proxy, sexp, &list, &err);
  g_free (sexp);
  if (!err) {
    GList *l = NULL;
    char **i = list;
    while (*i != NULL) {
      l = g_list_prepend (l, e_contact_new_from_vcard (*i++));
    }
    *contacts = g_list_reverse (l);
    g_strfreev (list);
    return TRUE;
  } else {
    return unwrap_gerror (err, error);
  }
}

static void
get_contacts_reply(DBusGProxy *proxy, char **vcards, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  GList *list = NULL;
  EBookListCallback cb = data->callback;
  if (vcards) {
    char **i = vcards;
    while (*i != NULL) {
      list = g_list_prepend (list, e_contact_new_from_vcard (*i++));
    }
  }
  list = g_list_reverse (list);
  if (cb)
    cb (data->book, get_status_from_error (error), list, data->closure);
  g_strfreev (vcards);
  g_free (data);
}

guint
e_book_async_get_contacts (EBook *book, EBookQuery *query, EBookListCallback cb, gpointer closure)
{
  struct async_data *data;
  char *sexp;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_val_if_fail (query, E_BOOK_ERROR_INVALID_ARG);

  sexp = e_book_query_to_string (query);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_get_contact_list_async (book->priv->proxy, sexp, get_contacts_reply, data);
  g_free (sexp);
  return 0;
}

static GList *
parse_changes_array (char **arr)
{
  GList *l = NULL;
  char **i;
  char *vcard;
  for (i = arr; *i != NULL; i++) {
    EBookChange *change = g_new (EBookChange, 1);
    /* TODO this is a bit of a hack */
    change->change_type = atoi (*i);
    vcard = strchr (*i, '\n') + 1;
    change->contact = e_contact_new_from_vcard (vcard);
    l = g_list_prepend (l, change);
  }
  g_strfreev (arr);
  return g_list_reverse (l);
}

gboolean
e_book_get_changes (EBook *book, char *changeid, GList **changes, GError **error)
{
  GError *err = NULL;
  char **list = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  org_gnome_evolution_dataserver_addressbook_Book_get_changes (book->priv->proxy, changeid, &list, &err);
  if (!err) {
    *changes = parse_changes_array (list);
    return TRUE;
  } else {
    return unwrap_gerror (err, error);
  }
}

static void
get_changes_reply(DBusGProxy *proxy, char **changes, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookListCallback cb = data->callback;
  GList *list;
  if (cb) {
    list = parse_changes_array (changes);
    cb (data->book, get_status_from_error (error), list, data->closure);
  }
  g_free (data);
}

guint
e_book_async_get_changes (EBook *book, char *changeid, EBookListCallback cb, gpointer closure)
{
  struct async_data *data;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_get_changes_async (book->priv->proxy, changeid, get_changes_reply, data);
  return 0;
}

void
e_book_free_change_list (GList *change_list)
{
	GList *l;
	for (l = change_list; l; l = l->next) {
		EBookChange *change = l->data;

		g_object_unref (change->contact);
		g_free (change);
	}

	g_list_free (change_list);
}

gboolean
e_book_add_contact (EBook *book, EContact *contact, GError **error)
{
  GError *err = NULL;
  char *vcard, *uid = NULL;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_error_if_fail (E_IS_CONTACT (contact), E_BOOK_ERROR_INVALID_ARG);

  vcard = e_vcard_to_string (E_VCARD (contact), EVC_FORMAT_VCARD_30);
  org_gnome_evolution_dataserver_addressbook_Book_add_contact (book->priv->proxy, vcard, &uid, &err);
  g_free (vcard);
  if (uid) {
    e_contact_set (contact, E_CONTACT_UID, uid);
    g_free (uid);
  }
  return unwrap_gerror (err, error);
}

static void
add_contact_reply (DBusGProxy *proxy, char *uid, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookIdCallback cb = data->callback;

  /* If there is an error returned the GLib bindings currently return garbage
     for the OUT values. This is bad. */
  if (error)
    uid = NULL;

  if (cb)
    cb (data->book, get_status_from_error (error), uid, data->closure);

  if (uid)
    g_free (uid);

  g_free (data);
}

gboolean
e_book_async_add_contact (EBook *book, EContact *contact, EBookIdCallback cb, gpointer closure)
{
  char *vcard;
  struct async_data *data;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_val_if_fail (E_IS_CONTACT (contact), E_BOOK_ERROR_INVALID_ARG);

  vcard = e_vcard_to_string (E_VCARD (contact), EVC_FORMAT_VCARD_30);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_add_contact_async (book->priv->proxy, vcard, add_contact_reply, data);
  g_free (vcard);
  return 0;
}

gboolean
e_book_commit_contact (EBook *book, EContact *contact, GError **error)
{
  GError *err = NULL;
  char *vcard;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_error_if_fail (E_IS_CONTACT (contact), E_BOOK_ERROR_INVALID_ARG);

  vcard = e_vcard_to_string (E_VCARD (contact), EVC_FORMAT_VCARD_30);
  org_gnome_evolution_dataserver_addressbook_Book_modify_contact (book->priv->proxy, vcard, &err);
  g_free (vcard);
  return unwrap_gerror (err, error);
}

static void
modify_contact_reply (DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  if (cb)
    cb (data->book, get_status_from_error (error), data->closure);
  g_free (data);
}

guint
e_book_async_commit_contact (EBook *book, EContact *contact, EBookCallback cb, gpointer closure)
{
  char *vcard;
  struct async_data *data;

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_if_fail (E_IS_CONTACT (contact), E_BOOK_ERROR_INVALID_ARG);

  vcard = e_vcard_to_string (E_VCARD (contact), EVC_FORMAT_VCARD_30);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_modify_contact_async (book->priv->proxy, vcard, modify_contact_reply, data);
  g_free (vcard);
  return 0;
}

gboolean
e_book_remove_contact (EBook *book, const char *id, GError **error)
{
  GError *err = NULL;
  const char *l[2];

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_error_if_fail (id, E_BOOK_ERROR_INVALID_ARG);

  l[0] = id;
  l[1] = NULL;

  org_gnome_evolution_dataserver_addressbook_Book_remove_contacts (book->priv->proxy, l, &err);
  return unwrap_gerror (err, error);
}

static void
remove_contact_reply (DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  if (cb)
    cb (data->book, get_status_from_error (error), data->closure);
  g_free (data);
}

guint
e_book_async_remove_contact (EBook *book, EContact *contact, EBookCallback cb, gpointer closure)
{
  struct async_data *data;
  const char *l[2];

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_if_fail (E_IS_CONTACT (contact), E_BOOK_ERROR_INVALID_ARG);

  l[0] = e_contact_get_const (contact, E_CONTACT_UID);
  l[1] = NULL;

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_remove_contacts_async (book->priv->proxy, l, remove_contact_reply, data);
  return 0;
}

static void
remove_contact_by_id_reply (DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  if (cb)
    cb (data->book, get_status_from_error (error), data->closure);
  g_free (data);
}

guint
e_book_async_remove_contact_by_id (EBook *book, const char *id, EBookCallback cb, gpointer closure)
{
  struct async_data *data;
  const char *l[2];

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_if_fail (id, E_BOOK_ERROR_INVALID_ARG);

  l[0] = id;
  l[1] = NULL;

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_remove_contacts_async (book->priv->proxy, l, remove_contact_by_id_reply, data);
  return 0;
}

gboolean
e_book_remove_contacts (EBook *book, GList *ids, GError **error)
{
  GError *err = NULL;
  char **l;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_error_if_fail (ids, E_BOOK_ERROR_INVALID_ARG);

  l = flatten_stringlist (ids);

  org_gnome_evolution_dataserver_addressbook_Book_remove_contacts (book->priv->proxy, (const char **) l, &err);
  g_free (l);
  return unwrap_gerror (err, error);
}

static void
remove_contacts_reply (DBusGProxy *proxy, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookCallback cb = data->callback;
  if (cb)
    cb (data->book, get_status_from_error (error), data->closure);
  g_free (data);
}

guint
e_book_async_remove_contacts (EBook *book, GList *id_list, EBookCallback cb, gpointer closure)
{
  struct async_data *data;
  char **l;

  e_return_async_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  if (id_list == NULL) {
    if (cb)
      cb (book, E_BOOK_ERROR_OK, closure);
    return 0;
  }

  l = flatten_stringlist (id_list);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  org_gnome_evolution_dataserver_addressbook_Book_remove_contacts_async (book->priv->proxy, (const char **) l, remove_contacts_reply, data);
  g_free (l);
  return 0;
}

gboolean
e_book_get_book_view (EBook *book, EBookQuery *query, GList *requested_fields, int max_results, EBookView **book_view, GError **error)
{
  GError *err = NULL;
  DBusGProxy *view_proxy;
  char **fields;
  char *sexp, *view_path;

  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  fields = flatten_stringlist (requested_fields);

  sexp = e_book_query_to_string (query);
  
  if (!org_gnome_evolution_dataserver_addressbook_Book_get_book_view (book->priv->proxy, sexp, (const char **) fields, max_results, &view_path, &err)) {
    *book_view = NULL;
    g_free (sexp);
    return unwrap_gerror (err, error);
  }
  view_proxy = dbus_g_proxy_new_for_name_owner (connection,
                                                E_DATA_BOOK_FACTORY_SERVICE_NAME, view_path,
                                                "org.gnome.evolution.dataserver.addressbook.BookView", error);
  /* TODO: handle failures properly here too */
  if (!view_proxy) {
    *book_view = NULL;
  } else {
    *book_view = e_book_view_new (book, view_proxy);
  }
  g_free (view_path);
  g_free (sexp);
  g_free (fields);
  return TRUE;
}

static void
get_book_view_reply (DBusGProxy *proxy, char *view_path, GError *error, gpointer user_data)
{
  struct async_data *data = user_data;
  EBookView *view;
  EBookBookViewCallback cb = data->callback;
  DBusGProxy *view_proxy;

  view_proxy = dbus_g_proxy_new_for_name_owner (connection, E_DATA_BOOK_FACTORY_SERVICE_NAME, view_path,
                                           "org.gnome.evolution.dataserver.addressbook.BookView", NULL);
  /* TODO: handle errors */
  view = e_book_view_new (data->book, view_proxy);

  if (cb)
    cb (data->book, get_status_from_error (error), view, data->closure);
  g_free (data);
}

guint
e_book_async_get_book_view (EBook *book, EBookQuery *query, GList *requested_fields, int max_results, EBookBookViewCallback cb, gpointer closure)
{
  struct async_data *data;
  char *sexp;
  char **fields;

  e_return_async_error_val_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_async_error_val_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  e_return_async_error_val_if_fail (query, E_BOOK_ERROR_INVALID_ARG);

  data = g_new (struct async_data, 1);
  data->book = book;
  data->callback = cb;
  data->closure = closure;

  fields = flatten_stringlist (requested_fields);
  sexp = e_book_query_to_string (query);

  org_gnome_evolution_dataserver_addressbook_Book_get_book_view_async (book->priv->proxy, sexp, (const char **) fields, max_results, get_book_view_reply, data);

  g_free (sexp);
  g_free (fields);
  return 0;
}

/**
 * e_book_is_opened:
 * @book: and #EBook
 *
 * Check if this book has been opened.
 *
 * Return value: %TRUE if this book has been opened, otherwise %FALSE.
 */
gboolean
e_book_is_opened (EBook *book)
{
  g_return_val_if_fail (E_IS_BOOK (book), FALSE);

  return book->priv->loaded;
}

/**
 * e_book_is_writable:
 * @book: an #EBook
 * 
 * Check if this book is writable.
 * 
 * Return value: %TRUE if this book is writable, otherwise %FALSE.
 */
gboolean
e_book_is_writable (EBook *book)
{
  g_return_val_if_fail (E_IS_BOOK (book), FALSE);
  return book->priv->writable;
}

/**
 * e_book_is_online:
 * @book: an #EBook
 *
 * Check if this book is connected.
 *
 * Return value: %TRUE if this book is connected, otherwise %FALSE.
 **/
gboolean 
e_book_is_online (EBook *book)
{
  g_return_val_if_fail (E_IS_BOOK (book), FALSE);
  
  return book->priv->connected;
}

/**
 * e_book_cancel:
 * @book: an #EBook
 * @error: a #GError to set on failure
 *
 * Used to cancel an already running operation on @book.  This
 * function makes a synchronous CORBA to the backend telling it to
 * cancel the operation.  If the operation wasn't cancellable (either
 * transiently or permanently) or had already comopleted on the server
 * side, this function will return E_BOOK_STATUS_COULD_NOT_CANCEL, and
 * the operation will continue uncancelled.  If the operation could be
 * cancelled, this function will return E_BOOK_ERROR_OK, and the
 * blocked e_book function corresponding to current operation will
 * return with a status of E_BOOK_STATUS_CANCELLED.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 **/
gboolean
e_book_cancel (EBook *book, GError **error)
{
  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);

  return org_gnome_evolution_dataserver_addressbook_Book_cancel_operation (book->priv->proxy, error);
}

/**
 * e_book_get_uri:
 * @book: an #EBook
 *
 * Get the URI that this book has loaded. This string should not be freed.
 *
 * Return value: The URI.
 */
const char *
e_book_get_uri (EBook *book)
{
  g_return_val_if_fail (E_IS_BOOK (book), NULL);

  return book->priv->uri;
}

/**
 * e_book_get_static_capabilities:
 * @book: an #EBook
 * @error: an #GError to set on failure
 *
 * Get the list of capabilities which the backend for this address book
 * supports. This string should not be freed.
 *
 * Return value: The capabilities list
 */
const char *
e_book_get_static_capabilities (EBook *book, GError **error)
{
  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->proxy, E_BOOK_ERROR_REPOSITORY_OFFLINE);
  
  if (!book->priv->cap_queried) {
    char *cap = NULL;
    
    if (!org_gnome_evolution_dataserver_addressbook_Book_get_static_capabilities (book->priv->proxy, &cap, error)) {
      return NULL;
    }

    book->priv->cap = cap;
    book->priv->cap_queried = TRUE;
  }
  
  return book->priv->cap;
}

#define SELF_UID_KEY "/apps/evolution/addressbook/self/self_uid"

/**
 * e_book_get_self:
 * @contact: an #EContact pointer to set
 * @book: an #EBook pointer to set
 * @error: a #GError to set on failure
 *
 * Get the #EContact referring to the user of the address book
 * and set it in @contact and @book.
 *
 * Return value: %TRUE if successful, otherwise %FALSE.
 **/
gboolean
e_book_get_self (EContact **contact, EBook **book, GError **error)
{
	GConfClient *gconf;
	gboolean status;
	char *uid;

	*book = e_book_new_system_addressbook (error);

	if (!*book) {
		return FALSE;
	}

	status = e_book_open (*book, FALSE, error);
	if (status == FALSE) {
		return FALSE;
	}

	gconf = gconf_client_get_default();
	uid = gconf_client_get_string (gconf, SELF_UID_KEY, NULL);
	g_object_unref (gconf);

	if (!uid) {
		g_object_unref (*book);
		*book = NULL;
		g_set_error (error, E_BOOK_ERROR, E_BOOK_ERROR_NO_SELF_CONTACT,
			     _("%s: there was no self contact uid stored in gconf"), "e_book_get_self");
		return FALSE;
	}

	if (!e_book_get_contact (*book, uid, contact, error)) {
		g_object_unref (*book);
		*book = NULL;
		return FALSE;
	}

	return TRUE;
}

/**
 * e_book_set_self:
 * @book: an #EBook
 * @contact: an #EContact
 * @error: a #GError to set on failure
 *
 * Specify that @contact residing in @book is the #EContact that
 * refers to the user of the address book.
 *
 * Return value: %TRUE if successful, %FALSE otherwise.
 **/
gboolean
e_book_set_self (EBook *book, EContact *contact, GError **error)
{
	GConfClient *gconf;

	e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
	e_return_error_if_fail (E_IS_CONTACT (contact), E_BOOK_ERROR_INVALID_ARG);

	gconf = gconf_client_get_default();
	gconf_client_set_string (gconf, SELF_UID_KEY, e_contact_get_const (contact, E_CONTACT_UID), NULL);
	g_object_unref (gconf);

	return TRUE;
}

/**
 * e_book_is_self:
 * @contact: an #EContact
 *
 * Check if @contact is the user of the address book.
 *
 * Return value: %TRUE if @contact is the user, %FALSE otherwise.
 **/
gboolean
e_book_is_self (EContact *contact)
{
	GConfClient *gconf;
	char *uid;
	gboolean rv;

	/* XXX this should probably be e_return_error_if_fail, but we
	   need a GError** arg for that */
	g_return_val_if_fail (contact && E_IS_CONTACT (contact), FALSE);

	gconf = gconf_client_get_default();
	uid = gconf_client_get_string (gconf, SELF_UID_KEY, NULL);
	g_object_unref (gconf);

	rv = (uid && strcmp (uid, e_contact_get_const (contact, E_CONTACT_UID)));

	g_free (uid);

	return rv;
}

/**
 * e_book_set_default_addressbook:
 * @book: An #EBook pointer
 * @error: A #GError pointer
 * 
 * sets the #ESource of the #EBook as the "default" addressbook.  This is the source
 * that will be loaded in the e_book_get_default_addressbook call.
 * 
 * Return value: %TRUE if the setting was stored in libebook's ESourceList, otherwise %FALSE.
 */
gboolean
e_book_set_default_addressbook (EBook *book, GError **error)
{
  ESource *source;
  
  e_return_error_if_fail (E_IS_BOOK (book), E_BOOK_ERROR_INVALID_ARG);
  e_return_error_if_fail (book->priv->loaded == FALSE, E_BOOK_ERROR_SOURCE_ALREADY_LOADED);
  
  source = e_book_get_source (book);
  
  return e_book_set_default_source (source, error);
}


/**
 * e_book_set_default_source:
 * @source: An #ESource pointer
 * @error: A #GError pointer
 * 
 * sets @source as the "default" addressbook.  This is the source that
 * will be loaded in the e_book_get_default_addressbook call.
 * 
 * Return value: %TRUE if the setting was stored in libebook's ESourceList, otherwise %FALSE.
 */
gboolean
e_book_set_default_source (ESource *source, GError **error)
{
	ESourceList *sources;
	const char *uid;
	GSList *g;

	e_return_error_if_fail (source && E_IS_SOURCE (source), E_BOOK_ERROR_INVALID_ARG);

	uid = e_source_peek_uid (source);

	if (!e_book_get_addressbooks (&sources, error)) {
		return FALSE;
	}

	/* make sure the source is actually in the ESourceList.  if
	   it's not we don't bother adding it, just return an error */
	source = e_source_list_peek_source_by_uid (sources, uid);
	if (!source) {
		g_set_error (error, E_BOOK_ERROR, E_BOOK_ERROR_NO_SUCH_SOURCE,
			     _("%s: there was no source for uid `%s' stored in gconf."), "e_book_set_default_source", uid);
		g_object_unref (sources);
		return FALSE;
	}

	/* loop over all the sources clearing out any "default"
	   properties we find */
	for (g = e_source_list_peek_groups (sources); g; g = g->next) {
		GSList *s;
		for (s = e_source_group_peek_sources (E_SOURCE_GROUP (g->data));
		     s; s = s->next) {
			e_source_set_property (E_SOURCE (s->data), "default", NULL);
		}
	}

	/* set the "default" property on the source */
	e_source_set_property (source, "default", "true");

	if (!e_source_list_sync (sources, error)) {
		return FALSE;
	}

	return TRUE;
}

/**
 * e_book_check_static_capability:
 * @book: an #EBook
 * @cap: A capability string
 *
 * Check to see if the backend for this address book supports the capability
 * @cap.
 *
 * Return value: %TRUE if the backend supports @cap, %FALSE otherwise.
 */
gboolean
e_book_check_static_capability (EBook *book, const char *cap)
{
  const char *caps;

  g_return_val_if_fail (E_IS_BOOK (book), FALSE);

  caps = e_book_get_static_capabilities (book, NULL);

  /* XXX this is an inexact test but it works for our use */
  if (caps && strstr (caps, cap))
    return TRUE;

  return FALSE;
}

/**
 * If the specified GError is a remote error, then create a new error
 * representing the remote error.  If the error is anything else, then leave it
 * alone.
 */
static gboolean
unwrap_gerror(GError *error, GError **client_error)
{
  if (error == NULL)
    return TRUE;
  if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
    GError *new;
    gint code;
    code = get_status_from_error (error);
    new = g_error_new_literal (E_BOOK_ERROR, code, error->message);
    g_error_free (error);
    if (client_error)
      *client_error = new;
  } else {
    if (client_error)
      *client_error = error;
  }
  return FALSE;
}

/**
 * If the GError is a remote error, extract the EBookStatus embedded inside.
 * Otherwise return CORBA_EXCEPTION (I know this is DBus...).
 */
static EBookStatus
get_status_from_error (GError *error)
{
  if G_LIKELY (error == NULL)
    return E_BOOK_ERROR_OK;
  if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION) {
    const char *name;
    name = dbus_g_error_get_name (error);
    if (strcmp (name, "org.gnome.evolution.dataserver.addressbook.Book.contactnotfound") == 0) {
      return E_BOOK_ERROR_CONTACT_NOT_FOUND;
    } else if (strcmp (name, "org.gnome.evolution.dataserver.addressbook.Book.invalidquery") == 0) {
      return E_BOOK_ERROR_INVALID_ARG;
    } else if (strcmp (name, "org.gnome.evolution.dataserver.addressbook.Book.cancelled") == 0) {
      return E_BOOK_ERROR_CANCELLED;
    } else if (strcmp (name, "org.gnome.evolution.dataserver.addressbook.Book.permissiondenied") == 0) {
      return E_BOOK_ERROR_PERMISSION_DENIED;
    } else if (strcmp (name, "org.gnome.evolution.dataserver.addressbook.Book.nospace") == 0) {
      return E_BOOK_ERROR_NO_SPACE;
    } else if (strcmp (name, "org.gnome.evolution.dataserver.addressbook.Book.othererror") == 0) {
      return E_BOOK_ERROR_OTHER_ERROR;
    } else {
      g_warning ("Unmatched error name %s", name);
      return E_BOOK_ERROR_OTHER_ERROR;
    }
  } else {
    /* In this case the error was caused by DBus */
    return E_BOOK_ERROR_CORBA_EXCEPTION;
  }
}

/**
 * Turn a GList of strings into an array of strings.
 */
static char **
flatten_stringlist (GList *list)
{
  char **array = g_new0 (char *, g_list_length (list) + 1);
  GList *l = list;
  int i = 0;
  while (l != NULL) {
    array[i++] = l->data;
    l = l->next;
  }
  return array;
}

/**
 * Turn an array of strings into a GList.
 */
static GList *
array_to_stringlist (char **list)
{
  GList *l = NULL;
  char **i = list;
  while (*i != NULL) {
    l = g_list_prepend (l, (*i++));
  }
  g_free (list);
  return g_list_reverse(l);
}