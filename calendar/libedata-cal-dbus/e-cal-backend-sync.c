/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Author:
 *   Chris Toshok (toshok@ximian.com)
 *
 * Copyright (C) 2003, Ximian, Inc.
 */

#ifdef CONFIG_H
#include <config.h>
#endif

#include "e-cal-backend-sync.h"

struct _ECalBackendSyncPrivate {
	GMutex *sync_mutex;

	gboolean mutex_lock;
};

#define LOCK_WRAPPER(func, args) \
  g_assert (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->func); \
  if (backend->priv->mutex_lock) \
    g_mutex_lock (backend->priv->sync_mutex); \
  status = (* E_CAL_BACKEND_SYNC_GET_CLASS (backend)->func) args; \
  if (backend->priv->mutex_lock) \
    g_mutex_unlock (backend->priv->sync_mutex); \
  return status;

static GObjectClass *parent_class;

/**
 * e_cal_backend_sync_set_lock:
 * @backend: An ECalBackendSync object.
 * @lock: Lock mode.
 *
 * Sets the lock mode on the ECalBackendSync object. If TRUE, the backend
 * will create a locking mutex for every operation, so that only one can
 * happen at a time. If FALSE, no lock would be done and many operations
 * can happen at the same time.
 */
void
e_cal_backend_sync_set_lock (ECalBackendSync *backend, gboolean lock)
{
	g_return_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend));

	backend->priv->mutex_lock = lock;
}

/**
 * e_cal_backend_sync_is_read_only:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @read_only: Return value for read-only status.
 *
 * Calls the is_read_only method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_is_read_only  (ECalBackendSync *backend, EDataCal *cal, gboolean *read_only)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
	g_return_val_if_fail (read_only, OtherError);

	LOCK_WRAPPER (is_read_only_sync, (backend, cal, read_only));

	return status;
}

/**
 * e_cal_backend_sync_get_cal_address:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @address: Return value for the address.
 *
 * Calls the get_cal_address method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_cal_address  (ECalBackendSync *backend, EDataCal *cal, char **address)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
	g_return_val_if_fail (address, OtherError);

	LOCK_WRAPPER (get_cal_address_sync, (backend, cal, address));

	return status;
}

/**
 * e_cal_backend_sync_get_alarm_email_address:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @address: Return value for the address.
 *
 * Calls the get_alarm_email_address method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_alarm_email_address  (ECalBackendSync *backend, EDataCal *cal, char **address)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
	g_return_val_if_fail (address, OtherError);

	LOCK_WRAPPER (get_alarm_email_address_sync, (backend, cal, address));

	return status;
}

/**
 * e_cal_backend_sync_get_ldap_attribute:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @attribute: Return value for LDAP attribute.
 *
 * Calls the get_ldap_attribute method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_ldap_attribute  (ECalBackendSync *backend, EDataCal *cal, char **attribute)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
	g_return_val_if_fail (attribute, OtherError);

	LOCK_WRAPPER (get_ldap_attribute_sync, (backend, cal, attribute));

	return status;
}

/**
 * e_cal_backend_sync_get_static_capabilities:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @capabilities: Return value for capabilities.
 *
 * Calls the get_capabilities method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_static_capabilities  (ECalBackendSync *backend, EDataCal *cal, char **capabilities)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
	g_return_val_if_fail (capabilities, OtherError);

	LOCK_WRAPPER (get_static_capabilities_sync, (backend, cal, capabilities));

	return status;
}

/**
 * e_cal_backend_sync_open:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @only_if_exists: Whether to open the calendar if and only if it already exists
 * or just create it when it does not exist.
 * @username: User name to use for authentication.
 * @password: Password to use for authentication.
 *
 * Calls the open method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_open  (ECalBackendSync *backend, EDataCal *cal, gboolean only_if_exists,
			  const char *username, const char *password)
{
	ECalBackendSyncStatus status;
	
	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);

	LOCK_WRAPPER (open_sync, (backend, cal, only_if_exists, username, password));

	return status;
}

/**
 * e_cal_backend_sync_remove:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 *
 * Calls the remove method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_remove  (ECalBackendSync *backend, EDataCal *cal)
{
 	ECalBackendSyncStatus status;
  
 	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  
 	LOCK_WRAPPER (remove_sync, (backend, cal));
 	
 	return status;
}

/**
 * e_cal_backend_sync_create_object:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @calobj: The object to be added.
 * @uid: Placeholder for server-generated UID.
 *
 * Calls the create_object method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_create_object (ECalBackendSync *backend, EDataCal *cal, char **calobj, char **uid)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->create_object_sync != NULL,
  			      UnsupportedMethod);
  
 	LOCK_WRAPPER (create_object_sync, (backend, cal, calobj, uid));
 
 	return status;
}

/**
 * e_cal_backend_sync_modify_object:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @calobj: Object to be modified.
 * @mod: Type of modification to be done.
 * @old_object: Placeholder for returning the old object as it was stored on the
 * backend.
 * @new_object: Placeholder for returning the new object as it has been stored
 * on the backend.
 *
 * Calls the modify_object method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_modify_object (ECalBackendSync *backend, EDataCal *cal, const char *calobj, 
				  CalObjModType mod, char **old_object, char **new_object)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->modify_object_sync != NULL,
  			      UnsupportedMethod);
  
 	LOCK_WRAPPER (modify_object_sync, (backend, cal, calobj, mod, old_object, new_object));
 
 	return status;
}

/**
 * e_cal_backend_sync_remove_object:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @uid: UID of the object to remove.
 * @rid: Recurrence ID of the instance to remove, or NULL if removing the
 * whole object.
 * @mod: Type of removal.
 * @old_object: Placeholder for returning the old object as it was stored on the
 * backend.
 * @object: Placeholder for returning the object after it has been modified (when
 * removing individual instances). If removing the whole object, this will be
 * NULL.
 *
 * Calls the remove_object method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_remove_object (ECalBackendSync *backend, EDataCal *cal, const char *uid, const char *rid,
				  CalObjModType mod, char **old_object, char **object)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->remove_object_sync != NULL,
  			      UnsupportedMethod);
  
 	LOCK_WRAPPER (remove_object_sync, (backend, cal, uid, rid, mod, old_object, object));
 
 	return status;
}

/**
 * e_cal_backend_sync_discard_alarm:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @uid: UID of the object to discard the alarm from.
 * @auid: UID of the alarm to be discarded.
 *
 * Calls the discard_alarm method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_discard_alarm (ECalBackendSync *backend, EDataCal *cal, const char *uid, const char *auid)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->discard_alarm_sync != NULL,
  			      UnsupportedMethod);
  
 	LOCK_WRAPPER (discard_alarm_sync, (backend, cal, uid, auid));
 
 	return status;
}

/**
 * e_cal_backend_sync_receive_objects:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @calobj: iCalendar object to receive.
 *
 * Calls the receive_objects method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_receive_objects (ECalBackendSync *backend, EDataCal *cal, const char *calobj)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->receive_objects_sync != NULL,
  			      UnsupportedMethod);
  
 	LOCK_WRAPPER (receive_objects_sync, (backend, cal, calobj));
 
 	return status;
}

/**
 * e_cal_backend_sync_send_objects:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @calobj: The iCalendar object to send.
 * @users: List of users to send notifications to.
 * @modified_calobj: Placeholder for the iCalendar object after being modified.
 *
 * Calls the send_objects method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_send_objects (ECalBackendSync *backend, EDataCal *cal, const char *calobj, GList **users,
				 char **modified_calobj)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (E_CAL_BACKEND_SYNC_GET_CLASS (backend)->send_objects_sync != NULL,
  			      UnsupportedMethod);
 
 	LOCK_WRAPPER (send_objects_sync, (backend, cal, calobj, users, modified_calobj));
 
 	return status;
}

/**
 * e_cal_backend_sync_get_default_object:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @object: Placeholder for returned object.
 *
 * Calls the get_default_object method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_default_object (ECalBackendSync *backend, EDataCal *cal, char **object)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (object, OtherError);
  
 	LOCK_WRAPPER (get_default_object_sync, (backend, cal, object));
  
 	return status;
}

/**
 * e_cal_backend_sync_get_object:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @uid: UID of the object to get.
 * @rid: Recurrence ID of the specific instance to get, or NULL if getting the
 * master object.
 * @object: Placeholder for returned object.
 *
 * Calls the get_object method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_object (ECalBackendSync *backend, EDataCal *cal, const char *uid, const char *rid, char **object)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (object, OtherError);
  
 	LOCK_WRAPPER (get_object_sync, (backend, cal, uid, rid, object));
  
 	return status;
}

/**
 * e_cal_backend_sync_get_object_list:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @sexp: Search query.
 * @objects: Placeholder for list of returned objects.
 *
 * Calls the get_object_list method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_object_list (ECalBackendSync *backend, EDataCal *cal, const char *sexp, GList **objects)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  	g_return_val_if_fail (objects, OtherError);
  
 	LOCK_WRAPPER (get_object_list_sync, (backend, cal, sexp, objects));
  
 	return status;
}

/**
 * e_cal_backend_sync_get_attachment_list:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @uid: Unique id of the calendar object.
 * @rid: Recurrence id of the calendar object.
 * @attachments: Placeholder for list of returned attachment uris.
 *
 * Calls the get_attachment_list method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_attachment_list (ECalBackendSync *backend, EDataCal *cal, const char *uid, const char *rid, GSList **attachments)
{
	ECalBackendSyncStatus status;
	
	g_return_val_if_fail (backend && E_IS_CAL_BACKEND_SYNC (backend), GNOME_Evolution_Calendar_OtherError);
	g_return_val_if_fail (attachments, GNOME_Evolution_Calendar_OtherError);
	LOCK_WRAPPER (get_attachment_list_sync, (backend, cal, uid, rid, attachments));
	
	return status;
}

/**
 * e_cal_backend_sync_get_timezone:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @tzid: ID of the timezone to retrieve.
 * @object: Placeholder for the returned timezone.
 *
 * Calls the get_timezone method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_timezone (ECalBackendSync *backend, EDataCal *cal, const char *tzid, char **object)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  
 	LOCK_WRAPPER (get_timezone_sync, (backend, cal, tzid, object));
  
 	return status;
}

/**
 * e_cal_backend_sync_add_timezone:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @tzobj: VTIMEZONE object to be added.
 *
 * Calls the add_timezone method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_add_timezone (ECalBackendSync *backend, EDataCal *cal, const char *tzobj)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  
 	LOCK_WRAPPER (add_timezone_sync, (backend, cal, tzobj));
  
 	return status;
}

/**
 * e_cal_backend_sync_set_default_timezone:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @tzid: ID of the timezone to be set as default.
 *
 * Calls the set_default_timezone method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_set_default_timezone (ECalBackendSync *backend, EDataCal *cal, const char *tzid)
{
 	ECalBackendSyncStatus status;
 
  	g_return_val_if_fail (E_IS_CAL_BACKEND_SYNC (backend), OtherError);
  
 	LOCK_WRAPPER (set_default_timezone_sync, (backend, cal, tzid));
  
 	return status;
}

/**
 * e_cal_backend_sync_get_changes:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @change_id: ID of the change to use as base.
 * @adds: Placeholder for list of additions.
 * @modifies: Placeholder for list of modifications.
 * @deletes: Placeholder for list of deletions.
 *
 * Calls the get_changes method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_changes (ECalBackendSync *backend, EDataCal *cal, const char *change_id,
				GList **adds, GList **modifies, GList **deletes)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (E_IS_CAL_BACKEND_SYNC (backend), OtherError);

	LOCK_WRAPPER (get_changes_sync, (backend, cal, change_id, adds, modifies, deletes));

	return status;
}

/**
 * e_cal_backend_sync_get_free_busy:
 * @backend: An ECalBackendSync object.
 * @cal: An EDataCal object.
 * @users: List of users to get F/B info from.
 * @start: Time range start.
 * @end: Time range end.
 * @freebusy: Placeholder for F/B information.
 *
 * Calls the get_free_busy method on the given backend.
 *
 * Return value: Status code.
 */
ECalBackendSyncStatus
e_cal_backend_sync_get_free_busy (ECalBackendSync *backend, EDataCal *cal, GList *users, 
				  time_t start, time_t end, GList **freebusy)
{
	ECalBackendSyncStatus status;

	g_return_val_if_fail (E_IS_CAL_BACKEND_SYNC (backend), OtherError);

	LOCK_WRAPPER (get_freebusy_sync, (backend, cal, users, start, end, freebusy));

	return status;
}

static void
_e_cal_backend_is_read_only (ECalBackend *backend, EDataCal *cal)
{
	ECalBackendSyncStatus status;
	gboolean read_only = TRUE;

	status = e_cal_backend_sync_is_read_only (E_CAL_BACKEND_SYNC (backend), cal, &read_only);

	e_data_cal_notify_read_only (cal, status, read_only);
}

static void
_e_cal_backend_get_cal_address (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context)
{
	ECalBackendSyncStatus status;
	char *address = NULL;

	status = e_cal_backend_sync_get_cal_address (E_CAL_BACKEND_SYNC (backend), cal, &address);

	e_data_cal_notify_cal_address (cal, context, status, address);

	g_free (address);
}

static void
_e_cal_backend_get_alarm_email_address (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context)
{
	ECalBackendSyncStatus status;
	char *address = NULL;

	status = e_cal_backend_sync_get_alarm_email_address (E_CAL_BACKEND_SYNC (backend), cal, &address);

	e_data_cal_notify_alarm_email_address (cal, context, status, address);

	g_free (address);
}

static void
_e_cal_backend_get_ldap_attribute (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context)
{
	ECalBackendSyncStatus status;
	char *attribute = NULL;

	status = e_cal_backend_sync_get_ldap_attribute (E_CAL_BACKEND_SYNC (backend), cal, &attribute);

	e_data_cal_notify_ldap_attribute (cal, context, status, attribute);

	g_free (attribute);
}

static void
_e_cal_backend_get_static_capabilities (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context)
{
	ECalBackendSyncStatus status;
	char *capabilities = NULL;

	status = e_cal_backend_sync_get_static_capabilities (E_CAL_BACKEND_SYNC (backend), cal, &capabilities);

	e_data_cal_notify_static_capabilities (cal, context, status, capabilities);

	g_free (capabilities);
}

static void
_e_cal_backend_open (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, gboolean only_if_exists,
		     const char *username, const char *password)
{
	ECalBackendSyncStatus status;

	status = e_cal_backend_sync_open (E_CAL_BACKEND_SYNC (backend), cal, only_if_exists, username, password);

	e_data_cal_notify_open (cal, context, status);
}

static void
_e_cal_backend_remove (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context)
{
	ECalBackendSyncStatus status;

	status = e_cal_backend_sync_remove (E_CAL_BACKEND_SYNC (backend), cal);

	e_data_cal_notify_remove (cal, context, status);
}

static void
_e_cal_backend_create_object (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *calobj)
{
	ECalBackendSyncStatus status;
	char *uid = NULL, *modified_calobj = (char *) calobj;
	
	status = e_cal_backend_sync_create_object (E_CAL_BACKEND_SYNC (backend), cal, &modified_calobj, &uid);

	e_data_cal_notify_object_created (cal, context, status, uid, modified_calobj);

	/* free memory */
	if (uid)
		g_free (uid);

	if (modified_calobj != calobj)
		g_free (modified_calobj);
}

static void
_e_cal_backend_modify_object (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *calobj, CalObjModType mod)
{
	ECalBackendSyncStatus status;
	char *old_object = NULL;
	char *new_object = NULL;
	
	status = e_cal_backend_sync_modify_object (E_CAL_BACKEND_SYNC (backend), cal, 
						   calobj, mod, &old_object, &new_object);

	if (new_object)
		e_data_cal_notify_object_modified (cal, context, status, old_object, new_object);
	else
		e_data_cal_notify_object_modified (cal, context, status, old_object, calobj);
}

static void
_e_cal_backend_remove_object (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *uid, const char *rid, CalObjModType mod)
{
	ECalBackendSyncStatus status;
	char *object = NULL, *old_object = NULL;
	
	status = e_cal_backend_sync_remove_object (E_CAL_BACKEND_SYNC (backend), cal, uid, rid, mod, &old_object, &object);

	if (status == Success) {
		char *calobj = NULL;

		if (e_cal_backend_sync_get_object (E_CAL_BACKEND_SYNC (backend), cal, uid, NULL, &calobj)
		    == Success) {
			e_data_cal_notify_object_modified (cal, context, status, old_object, calobj);
			g_free (calobj);
		} else
			e_data_cal_notify_object_removed (cal, context, status, uid, old_object, object);
	} else
		e_data_cal_notify_object_removed (cal, context, status, uid, old_object, object);

	g_free (object);
}

static void
_e_cal_backend_discard_alarm (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *uid, const char *auid)
{
	ECalBackendSyncStatus status;
	
	status = e_cal_backend_sync_discard_alarm (E_CAL_BACKEND_SYNC (backend), cal, uid, auid);

	e_data_cal_notify_alarm_discarded (cal, context, status);
}

static void
_e_cal_backend_receive_objects (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *calobj)
{
	ECalBackendSyncStatus status;
	
	status = e_cal_backend_sync_receive_objects (E_CAL_BACKEND_SYNC (backend), cal, calobj);

	e_data_cal_notify_objects_received (cal, context, status);
}

static void
_e_cal_backend_send_objects (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *calobj)
{
	ECalBackendSyncStatus status;
	GList *users = NULL;
	char *modified_calobj = NULL;

	status = e_cal_backend_sync_send_objects (E_CAL_BACKEND_SYNC (backend), cal, calobj, &users, &modified_calobj);
	e_data_cal_notify_objects_sent (cal, context, status, users, modified_calobj);

	g_list_foreach (users, (GFunc) g_free, NULL);
	g_list_free (users);
	g_free (modified_calobj);
}

static void
_e_cal_backend_get_default_object (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context)
{
	ECalBackendSyncStatus status;
	char *object = NULL;

	status = e_cal_backend_sync_get_default_object (E_CAL_BACKEND_SYNC (backend), cal, &object);

	e_data_cal_notify_default_object (cal, context, status, object);

	g_free (object);
}

static void
_e_cal_backend_get_object (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *uid, const char *rid)
{
	ECalBackendSyncStatus status;
	char *object = NULL;

	status = e_cal_backend_sync_get_object (E_CAL_BACKEND_SYNC (backend), cal, uid, rid, &object);

	e_data_cal_notify_object (cal, context, status, object);
	
	g_free (object);
}

static void
_e_cal_backend_get_attachment_list (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *uid, const char *rid)
{
	ECalBackendSyncStatus status;
	GSList *list = NULL;
	
	status = e_cal_backend_sync_get_attachment_list (E_CAL_BACKEND_SYNC (backend), cal, uid, rid, &list);
	
	e_data_cal_notify_attachment_list (cal, context, status, list);
	
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_free (list);
}

static void
_e_cal_backend_get_object_list (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *sexp)
{
	ECalBackendSyncStatus status;
	GList *objects = NULL, *l;

	status = e_cal_backend_sync_get_object_list (E_CAL_BACKEND_SYNC (backend), cal, sexp, &objects);

	e_data_cal_notify_object_list (cal, context, status, objects);

	for (l = objects; l; l = l->next)
		g_free (l->data);
	g_list_free (objects);
}

static void
_e_cal_backend_get_timezone (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *tzid)
{
	ECalBackendSyncStatus status;
	char *object = NULL;
	
	status = e_cal_backend_sync_get_timezone (E_CAL_BACKEND_SYNC (backend), cal, tzid, &object);

	e_data_cal_notify_timezone_requested (cal, context, status, object);

	g_free (object);
}

static void
_e_cal_backend_add_timezone (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *tzobj)
{
	ECalBackendSyncStatus status;

	status = e_cal_backend_sync_add_timezone (E_CAL_BACKEND_SYNC (backend), cal, tzobj);

	e_data_cal_notify_timezone_added (cal, context, status, tzobj);
}

static void
_e_cal_backend_set_default_timezone (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *tzid)
{
	ECalBackendSyncStatus status;

	status = e_cal_backend_sync_set_default_timezone (E_CAL_BACKEND_SYNC (backend), cal, tzid);

	e_data_cal_notify_default_timezone_set (cal, context, status);
}

static void
_e_cal_backend_get_changes (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, const char *change_id)
{
	ECalBackendSyncStatus status;
	GList *adds = NULL, *modifies = NULL, *deletes = NULL, *l;
	
	status = e_cal_backend_sync_get_changes (E_CAL_BACKEND_SYNC (backend), cal, change_id, 
					       &adds, &modifies, &deletes);

	e_data_cal_notify_changes (cal, context, status, adds, modifies, deletes);

	for (l = adds; l; l = l->next)
		g_free (l->data);
	g_list_free (adds);

	for (l = modifies; l; l = l->next)
		g_free (l->data);
	g_list_free (modifies);

	for (l = deletes; l; l = l->next)
		g_free (l->data);
	g_list_free (deletes);
}

static void
_e_cal_backend_get_free_busy (ECalBackend *backend, EDataCal *cal, DBusGMethodInvocation *context, GList *users, time_t start, time_t end)
{
	ECalBackendSyncStatus status;
	GList *freebusy = NULL, *l;
	
	status = e_cal_backend_sync_get_free_busy (E_CAL_BACKEND_SYNC (backend), cal, users, start, end, &freebusy);

	e_data_cal_notify_free_busy (cal, context, status, freebusy);

	for (l = freebusy; l; l = l->next)
		g_free (l->data);
	g_list_free (freebusy);
}

static void
e_cal_backend_sync_init (ECalBackendSync *backend)
{
	ECalBackendSyncPrivate *priv;

	priv             = g_new0 (ECalBackendSyncPrivate, 1);
	priv->sync_mutex = g_mutex_new ();

	backend->priv = priv;
}

static void
e_cal_backend_sync_dispose (GObject *object)
{
	ECalBackendSync *backend;

	backend = E_CAL_BACKEND_SYNC (object);

	if (backend->priv) {
		g_mutex_free (backend->priv->sync_mutex);
		g_free (backend->priv);

		backend->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
e_cal_backend_sync_class_init (ECalBackendSyncClass *klass)
{
	GObjectClass *object_class;
	ECalBackendClass *backend_class = E_CAL_BACKEND_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class = (GObjectClass *) klass;

	backend_class->is_read_only = _e_cal_backend_is_read_only;
	backend_class->get_cal_address = _e_cal_backend_get_cal_address;
	backend_class->get_alarm_email_address = _e_cal_backend_get_alarm_email_address;
	backend_class->get_ldap_attribute = _e_cal_backend_get_ldap_attribute;
	backend_class->get_static_capabilities = _e_cal_backend_get_static_capabilities;
	backend_class->open = _e_cal_backend_open;
	backend_class->remove = _e_cal_backend_remove;
	backend_class->create_object = _e_cal_backend_create_object;
	backend_class->modify_object = _e_cal_backend_modify_object;
	backend_class->remove_object = _e_cal_backend_remove_object;
	backend_class->discard_alarm = _e_cal_backend_discard_alarm;
	backend_class->receive_objects = _e_cal_backend_receive_objects;
	backend_class->send_objects = _e_cal_backend_send_objects;
	backend_class->get_default_object = _e_cal_backend_get_default_object;
	backend_class->get_object = _e_cal_backend_get_object;
	backend_class->get_object_list = _e_cal_backend_get_object_list;
	backend_class->get_attachment_list = _e_cal_backend_get_attachment_list;
	backend_class->get_timezone = _e_cal_backend_get_timezone;
	backend_class->add_timezone = _e_cal_backend_add_timezone;
	backend_class->set_default_timezone = _e_cal_backend_set_default_timezone;
 	backend_class->get_changes = _e_cal_backend_get_changes;
 	backend_class->get_free_busy = _e_cal_backend_get_free_busy;

	object_class->dispose = e_cal_backend_sync_dispose;
}

/**
 * e_cal_backend_get_type:
 *
 * Registers the ECalBackendSync class if needed.
 *
 * Return value: The ID of the ECalBackendSync class.
 */
GType
e_cal_backend_sync_get_type (void)
{
	static GType type = 0;

	if (! type) {
		GTypeInfo info = {
			sizeof (ECalBackendSyncClass),
			NULL, /* base_class_init */
			NULL, /* base_class_finalize */
			(GClassInitFunc)  e_cal_backend_sync_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (ECalBackendSync),
			0,    /* n_preallocs */
			(GInstanceInitFunc) e_cal_backend_sync_init
		};

		type = g_type_register_static (E_TYPE_CAL_BACKEND, "ECalBackendSync", &info, 0);
	}

	return type;
}
