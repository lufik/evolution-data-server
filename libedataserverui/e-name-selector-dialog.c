/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* e-name-selector-dialog.c - Dialog that lets user pick EDestinations.
 *
 * Copyright (C) 2004 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Hans Petter Jansson <hpj@novell.com>
 */

#include <config.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkstock.h>
#include <glib/gi18n-lib.h>
#include <libedataserverui/e-source-option-menu.h>
#include <libedataserverui/e-destination-store.h>
#include <libedataserverui/e-contact-store.h>
#include <libedataserverui/e-book-auth-util.h>
#include <libedataserver/e-sexp.h>
#include "e-name-selector-dialog.h"

typedef struct {
	gchar        *name;

	GtkBox       *section_box;
	GtkLabel     *label;  
	GtkButton    *transfer_button;
	GtkButton    *remove_button;
	GtkTreeView  *destination_view;
}
Section;

typedef struct {
	GtkTreeView *view;
	GtkButton   *button;
	ENameSelectorDialog *dlg_ptr;
} SelData;

static ESource *find_first_source             (ESourceList *source_list);
static void     search_changed                (ENameSelectorDialog *name_selector_dialog);
static void     source_selected               (ENameSelectorDialog *name_selector_dialog, ESource *source);
static void     transfer_button_clicked       (ENameSelectorDialog *name_selector_dialog, GtkButton *transfer_button);
static void     contact_selection_changed     (ENameSelectorDialog *name_selector_dialog);
static void     setup_name_selector_model     (ENameSelectorDialog *name_selector_dialog);
static void     contact_activated             (ENameSelectorDialog *name_selector_dialog, GtkTreePath *path);
static void     destination_activated         (ENameSelectorDialog *name_selector_dialog, GtkTreePath *path,
					       GtkTreeViewColumn *column, GtkTreeView *tree_view);
static gboolean destination_key_press         (ENameSelectorDialog *name_selector_dialog, GdkEventKey *event, GtkTreeView *tree_view);
static void remove_button_clicked (GtkButton *button, SelData *data);
static void     remove_books                  (ENameSelectorDialog *name_selector_dialog);
static void     contact_column_formatter      (GtkTreeViewColumn *column, GtkCellRenderer *cell,
					       GtkTreeModel *model, GtkTreeIter *iter,
					       ENameSelectorDialog *name_selector_dialog);
static void     destination_column_formatter  (GtkTreeViewColumn *column, GtkCellRenderer *cell,
					       GtkTreeModel *model, GtkTreeIter *iter,
					       ENameSelectorDialog *name_selector_dialog);

/* ------------------ *
 * Class/object setup *
 * ------------------ */

G_DEFINE_TYPE (ENameSelectorDialog, e_name_selector_dialog, GTK_TYPE_DIALOG);

static void
e_name_selector_dialog_get_property (GObject *object, guint prop_id,
				     GValue *value, GParamSpec *pspec)
{
}

static void
e_name_selector_dialog_set_property (GObject *object, guint prop_id,
				     const GValue *value, GParamSpec *pspec)
{
}

static void
e_name_selector_dialog_init (ENameSelectorDialog *name_selector_dialog)
{
	GtkTreeSelection  *contact_selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *cell_renderer;
	GtkWidget         *widget;
	GtkWidget         *container;
	GtkWidget	  *label;
	GtkTreeSelection  *selection;
	ESourceList       *source_list;

	/* Get Glade GUI */

	name_selector_dialog->gui = glade_xml_new (E_DATA_SERVER_UI_GLADEDIR "/e-name-selector-dialog.glade", NULL, GETTEXT_PACKAGE);

	widget = glade_xml_get_widget (name_selector_dialog->gui, "name-selector-box");
	if (!widget) {
		g_warning ("ENameSelectorDialog can't load Glade interface!");
		g_object_unref (name_selector_dialog->gui);
		name_selector_dialog->gui = NULL;
		return;
	}

	/* Get addressbook sources */

	if (!e_book_get_addressbooks (&source_list, NULL)) {
		g_warning ("ENameSelectorDialog can't find any addressbooks!");
		g_object_unref (name_selector_dialog->gui);
		return;
	}

	/* Reparent it to inside ourselves */

	g_object_ref (widget);
	gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (name_selector_dialog)->vbox), widget, TRUE, TRUE, 0);
	g_object_unref (widget);

	/* Store pointers to relevant widgets */

	name_selector_dialog->contact_view = GTK_TREE_VIEW (
		glade_xml_get_widget (name_selector_dialog->gui, "source-tree-view"));
	name_selector_dialog->status_label = GTK_LABEL (
		glade_xml_get_widget (name_selector_dialog->gui, "status-message"));
	name_selector_dialog->destination_box = GTK_BOX (
		glade_xml_get_widget (name_selector_dialog->gui, "destination-box"));
	name_selector_dialog->search_entry = GTK_ENTRY (
		glade_xml_get_widget (name_selector_dialog->gui, "search"));

	/* Create size group for transfer buttons */

	name_selector_dialog->button_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Set up contacts view */

	column = gtk_tree_view_column_new ();
	cell_renderer = GTK_CELL_RENDERER (gtk_cell_renderer_text_new ());
	gtk_tree_view_column_pack_start (column, cell_renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, cell_renderer,
						 (GtkTreeCellDataFunc) contact_column_formatter,
						 name_selector_dialog, NULL);

	selection = gtk_tree_view_get_selection (name_selector_dialog->contact_view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	gtk_tree_view_append_column (name_selector_dialog->contact_view, column);
	g_signal_connect_swapped (name_selector_dialog->contact_view, "row-activated",
				  G_CALLBACK (contact_activated), name_selector_dialog);

	/* Listen for changes to the contact selection */

	contact_selection = gtk_tree_view_get_selection (name_selector_dialog->contact_view);
	g_signal_connect_swapped (contact_selection, "changed",
				  G_CALLBACK (contact_selection_changed), name_selector_dialog);

	/* Set up our data structures */

	name_selector_dialog->name_selector_model = e_name_selector_model_new ();
	name_selector_dialog->sections            = g_array_new (FALSE, FALSE, sizeof (Section));
	name_selector_dialog->source_list         = source_list;

	setup_name_selector_model (name_selector_dialog);

	/* Create source menu */

	widget = e_source_option_menu_new (name_selector_dialog->source_list);
	g_signal_connect_swapped (widget, "source_selected", G_CALLBACK (source_selected), name_selector_dialog);

	label = glade_xml_get_widget (name_selector_dialog->gui, "AddressBookLabel");
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);

	gtk_widget_show (widget);

	container = glade_xml_get_widget (name_selector_dialog->gui, "source-menu-box");
	gtk_box_pack_start (GTK_BOX (container), widget, TRUE, TRUE, 0);

	/* Set up search-as-you-type signal */

	widget = glade_xml_get_widget (name_selector_dialog->gui, "search");
	g_signal_connect_swapped (widget, "changed", G_CALLBACK (search_changed), name_selector_dialog);

	/* Display initial source */

	/* TODO: Remember last used source */

	source_selected (name_selector_dialog, find_first_source (name_selector_dialog->source_list));

	/* Set up dialog defaults */

	gtk_dialog_add_buttons (GTK_DIALOG (name_selector_dialog),
				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (name_selector_dialog), GTK_RESPONSE_CLOSE);
	gtk_window_set_modal            (GTK_WINDOW (name_selector_dialog), TRUE);
	gtk_window_set_default_size     (GTK_WINDOW (name_selector_dialog), 472, 512);
	gtk_window_set_resizable        (GTK_WINDOW (name_selector_dialog), TRUE);
	gtk_dialog_set_has_separator    (GTK_DIALOG (name_selector_dialog), FALSE);
	gtk_container_set_border_width  (GTK_CONTAINER (name_selector_dialog), 4);
	gtk_window_set_title            (GTK_WINDOW (name_selector_dialog), _("Select Contacts from Address Book"));
}

/* Partial, repeatable destruction. Release references. */
static void
e_name_selector_dialog_dispose (GObject *object)
{
	ENameSelectorDialog *name_selector_dialog = E_NAME_SELECTOR_DIALOG (object);

	g_signal_handlers_disconnect_matched (name_selector_dialog->name_selector_model,
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL,
					      name_selector_dialog);

	remove_books (name_selector_dialog);

	if (G_OBJECT_CLASS (e_name_selector_dialog_parent_class)->dispose)
		G_OBJECT_CLASS (e_name_selector_dialog_parent_class)->dispose (object);
}

/* Final, one-time destruction. Free all. */
static void
e_name_selector_dialog_finalize (GObject *object)
{
	/* TODO: Free stuff */

	if (G_OBJECT_CLASS (e_name_selector_dialog_parent_class)->finalize)
		G_OBJECT_CLASS (e_name_selector_dialog_parent_class)->finalize (object);
}

static void
e_name_selector_dialog_class_init (ENameSelectorDialogClass *name_selector_dialog_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (name_selector_dialog_class);

	object_class->get_property = e_name_selector_dialog_get_property;
	object_class->set_property = e_name_selector_dialog_set_property;
	object_class->dispose      = e_name_selector_dialog_dispose;
	object_class->finalize     = e_name_selector_dialog_finalize;

	/* Install properties */

	/* Install signals */

}

/**
 * e_name_selector_dialog_new:
 *
 * Creates a new #ENameSelectorDialog.
 *
 * Return value: A new #ENameSelectorDialog.
 **/
ENameSelectorDialog *
e_name_selector_dialog_new (void)
{
	  return g_object_new (E_TYPE_NAME_SELECTOR_DIALOG, NULL);
}

/* --------- *
 * Utilities *
 * --------- */

static gchar *
escape_sexp_string (const gchar *string)
{
	GString *gstring;
	gchar   *encoded_string;

	gstring = g_string_new ("");
	e_sexp_encode_string (gstring, string);

	encoded_string = gstring->str;
	g_string_free (gstring, FALSE);

	return encoded_string;
}

static void
sort_iter_to_contact_store_iter (ENameSelectorDialog *name_selector_dialog, GtkTreeIter *iter,
				 gint *email_n)
{
	ETreeModelGenerator *contact_filter;
	GtkTreeIter          child_iter;
	gint                 email_n_local;

	contact_filter = e_name_selector_model_peek_contact_filter (name_selector_dialog->name_selector_model);

	gtk_tree_model_sort_convert_iter_to_child_iter (name_selector_dialog->contact_sort,
							&child_iter, iter);
	e_tree_model_generator_convert_iter_to_child_iter (contact_filter, iter, &email_n_local, &child_iter);

	if (email_n)
		*email_n = email_n_local;
}

static ESource *
find_first_source (ESourceList *source_list)
{
	GSList *groups, *sources, *l, *m;
			
	groups = e_source_list_peek_groups (source_list);
	for (l = groups; l; l = l->next) {
		ESourceGroup *group = l->data;
				
		sources = e_source_group_peek_sources (group);
		for (m = sources; m; m = m->next) {
			ESource *source = m->data;

			return source;
		}				
	}

	return NULL;
}

static void
add_destination (EDestinationStore *destination_store, EContact *contact, gint email_n)
{
	EDestination *destination;

	/* Transfer (actually, copy into a destination and let the model filter out the
	 * source automatically) */

	destination = e_destination_new ();
	e_destination_set_contact (destination, contact, email_n);
	e_destination_store_append_destination (destination_store, destination);
	g_object_unref (destination);
}

static void
remove_books (ENameSelectorDialog *name_selector_dialog)
{
	EContactStore *contact_store;
	GList         *books;
	GList         *l;

	contact_store = e_name_selector_model_peek_contact_store (name_selector_dialog->name_selector_model);

	/* Remove books (should be just one) being viewed */
	books = e_contact_store_get_books (contact_store);
	for (l = books; l; l = g_list_next (l)) {
		EBook *book = l->data;
		e_contact_store_remove_book (contact_store, book);
	}
	g_list_free (books);

	/* See if we have a book pending; stop loading it if so */
	if (name_selector_dialog->pending_book) {
		e_book_cancel (name_selector_dialog->pending_book, NULL);
		g_object_unref (name_selector_dialog->pending_book);
		name_selector_dialog->pending_book = NULL;
	}
}

/* ------------------ *
 * Section management *
 * ------------------ */

static gint
find_section_by_transfer_button (ENameSelectorDialog *name_selector_dialog, GtkButton *transfer_button)
{
	gint i;

	for (i = 0; i < name_selector_dialog->sections->len; i++) {
		Section *section = &g_array_index (name_selector_dialog->sections, Section, i);

		if (section->transfer_button == transfer_button)
			return i;
	}

	return -1;
}

static gint
find_section_by_tree_view (ENameSelectorDialog *name_selector_dialog, GtkTreeView *tree_view)
{
	gint i;

	for (i = 0; i < name_selector_dialog->sections->len; i++) {
		Section *section = &g_array_index (name_selector_dialog->sections, Section, i);

		if (section->destination_view == tree_view)
			return i;
	}

	return -1;
}

static gint
find_section_by_name (ENameSelectorDialog *name_selector_dialog, const gchar *name)
{
	gint i;

	for (i = 0; i < name_selector_dialog->sections->len; i++) {
		Section *section = &g_array_index (name_selector_dialog->sections, Section, i);

		if (!strcmp (name, section->name))
			return i;
	}

	return -1;
}

static void
selection_changed (GtkTreeSelection *selection, SelData *data)
{
	GtkTreeSelection *contact_selection;
	gboolean          have_selection = FALSE;

	contact_selection = gtk_tree_view_get_selection (data->view);
	if (gtk_tree_selection_count_selected_rows (contact_selection) > 0)
		have_selection = TRUE;
	gtk_widget_set_sensitive (GTK_WIDGET (data->button), have_selection);
}

static gint
add_section (ENameSelectorDialog *name_selector_dialog,
	     const gchar *name, const gchar *pretty_name, EDestinationStore *destination_store)
{
	Section            section;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *cell_renderer;
	GtkWidget  	  *vbox, *hbox, *chbox;
	GtkWidget 	  *widget, *image, *label;
	SelData		  *data;
	GtkTreeSelection  *selection;
	gchar 		  *text;

	g_assert (name != NULL);
	g_assert (pretty_name != NULL);
	g_assert (E_IS_DESTINATION_STORE (destination_store));

	memset (&section, 0, sizeof (Section));

	section.name = g_strdup (name);
	section.section_box      = GTK_BOX (gtk_hbox_new (FALSE, 12));
	section.label = GTK_LABEL (gtk_label_new_with_mnemonic (pretty_name));
	section.transfer_button  = GTK_BUTTON (gtk_button_new());
	section.remove_button  = GTK_BUTTON (gtk_button_new());
	section.destination_view = GTK_TREE_VIEW (gtk_tree_view_new ());

	if (pango_parse_markup (pretty_name, -1, '_', NULL,
				&text, NULL, NULL))  {
		atk_object_set_name (gtk_widget_get_accessible (
					GTK_WIDGET (section.destination_view)), text);
		g_free (text);
	}

	/* Set up transfer button */
	g_signal_connect_swapped (section.transfer_button, "clicked",
				  G_CALLBACK (transfer_button_clicked), name_selector_dialog);
	
	/*data for the remove callback*/
	data = g_malloc0(sizeof(SelData));
	data->view = section.destination_view;
	data->dlg_ptr = name_selector_dialog;

	/*Associate to an object destroy so that it gets freed*/
	g_object_set_data_full ((GObject *)section.destination_view, "sel-remove-data", data, g_free);
	
	g_signal_connect(section.remove_button, "clicked",
				  G_CALLBACK (remove_button_clicked), data);
	
	/* Set up view */
	column = gtk_tree_view_column_new ();
	cell_renderer = GTK_CELL_RENDERER (gtk_cell_renderer_text_new ());
	gtk_tree_view_column_pack_start (column, cell_renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, cell_renderer,
						 (GtkTreeCellDataFunc) destination_column_formatter,
						 name_selector_dialog, NULL);
	gtk_tree_view_append_column (section.destination_view, column);
	gtk_tree_view_set_headers_visible (section.destination_view, FALSE);
	gtk_tree_view_set_model (section.destination_view, GTK_TREE_MODEL (destination_store));

	vbox = gtk_vbox_new (FALSE, 0);
	chbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (chbox), vbox, FALSE, FALSE, 12);

	/* Pack button (in an alignment) */
	widget = gtk_alignment_new (0.5, 0.0, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (section.transfer_button));
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 6);
	gtk_size_group_add_widget (name_selector_dialog->button_size_group, GTK_WIDGET (section.transfer_button));
	
	/*to get the image embedded in the button*/
	widget = gtk_alignment_new (0.7, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (section.transfer_button), GTK_WIDGET (widget));

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (GTK_WIDGET(hbox));
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET(hbox));

	label = gtk_label_new_with_mnemonic (_("_Add"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	image = gtk_image_new_from_stock ("gtk-go-forward", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	
	widget = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (section.remove_button));
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
	gtk_size_group_add_widget (name_selector_dialog->button_size_group, GTK_WIDGET (section.remove_button));
	gtk_widget_set_sensitive (GTK_WIDGET (section.remove_button), FALSE);

	widget = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (section.remove_button), widget);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (widget), hbox);

	image = gtk_image_new_from_stock ("gtk-go-back", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Remove"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	
	widget = gtk_alignment_new (0.5, 0.0, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (section.label));
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);

	/* Pack view (in a scrolled window) */
	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (widget), GTK_WIDGET (section.destination_view));
	gtk_box_pack_start (GTK_BOX (chbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), chbox, TRUE, TRUE, 0);
	gtk_box_pack_start (section.section_box, vbox, TRUE, TRUE, 0);

	/*data for 'changed' callback*/
	data = g_malloc0(sizeof(SelData));
	data->view = section.destination_view;
	data->button = section.remove_button;
	g_object_set_data_full ((GObject *)section.destination_view, "sel-change-data", data, g_free);
	selection = gtk_tree_view_get_selection(section.destination_view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect(selection, "changed",
				  G_CALLBACK (selection_changed), data);

	g_signal_connect_swapped (section.destination_view, "row-activated",
				  G_CALLBACK (destination_activated), name_selector_dialog);
	g_signal_connect_swapped (section.destination_view, "key-press-event",
				  G_CALLBACK (destination_key_press), name_selector_dialog);

	gtk_widget_show_all (GTK_WIDGET (section.section_box));

	/* Pack this section's box into the dialog */
	gtk_box_pack_start (name_selector_dialog->destination_box,
			    GTK_WIDGET (section.section_box), TRUE, TRUE, 0);

	g_array_append_val (name_selector_dialog->sections, section);

	/* Make sure UI is consistent */
	contact_selection_changed (name_selector_dialog);

	return name_selector_dialog->sections->len - 1;
}

static void
free_section (ENameSelectorDialog *name_selector_dialog, gint n)
{
	Section *section;

	g_assert (n >= 0);
	g_assert (n < name_selector_dialog->sections->len);

	section = &g_array_index (name_selector_dialog->sections, Section, n);

	g_free (section->name);
	gtk_widget_destroy (GTK_WIDGET (section->section_box));
}

static void
model_section_added (ENameSelectorDialog *name_selector_dialog, const gchar *name)
{
	gchar             *pretty_name;
	EDestinationStore *destination_store;

	e_name_selector_model_peek_section (name_selector_dialog->name_selector_model, name,
					    &pretty_name, &destination_store);
	add_section (name_selector_dialog, name, pretty_name, destination_store);
	g_free (pretty_name);
}

static void
model_section_removed (ENameSelectorDialog *name_selector_dialog, const gchar *name)
{
	gint section_index;

	section_index = find_section_by_name (name_selector_dialog, name);
	g_assert (section_index >= 0);

	free_section (name_selector_dialog, section_index);
	g_array_remove_index (name_selector_dialog->sections, section_index);
}

/* -------------------- *
 * Addressbook selector *
 * -------------------- */

static void
book_opened (EBook *book, EBookStatus status, gpointer data)
{
	ENameSelectorDialog *name_selector_dialog = E_NAME_SELECTOR_DIALOG (data);
	EContactStore       *contact_store;

	if (status != E_BOOK_ERROR_OK) {
		/* TODO: Handle errors gracefully */
		g_warning ("ENameSelectorDialog failed to open book!");
		return;
	}

	contact_store = e_name_selector_model_peek_contact_store (name_selector_dialog->name_selector_model);
	e_contact_store_add_book (contact_store, book);
	g_object_unref (book);
	name_selector_dialog->pending_book = NULL;
}

static void
source_selected (ENameSelectorDialog *name_selector_dialog, ESource *source)
{
	/* Remove any previous books being shown or loaded */
	remove_books (name_selector_dialog);

	/* Start loading selected book */
	name_selector_dialog->pending_book = e_load_book_source (source, book_opened,
								 name_selector_dialog);
}

/* --------------- *
 * Other UI events *
 * --------------- */

static void
search_changed (ENameSelectorDialog *name_selector_dialog)
{
	EContactStore *contact_store;
	EBookQuery    *book_query;
	const gchar   *text;
	gchar         *text_escaped;
	gchar         *query_string;

	text = gtk_entry_get_text (name_selector_dialog->search_entry);
	text_escaped = escape_sexp_string (text);
	query_string = g_strdup_printf ("(or (beginswith \"file_as\" %s) "
					"    (beginswith \"full_name\" %s) "
					"    (beginswith \"email\" %s) "
					"    (beginswith \"nickname\" %s))",
					text_escaped, text_escaped, text_escaped, text_escaped);
	book_query = e_book_query_from_string (query_string);
	g_free (query_string);
	g_free (text_escaped);

	contact_store = e_name_selector_model_peek_contact_store (name_selector_dialog->name_selector_model);
	e_contact_store_set_query (contact_store, book_query);

	e_book_query_unref (book_query);
}

static void
contact_selection_changed (ENameSelectorDialog *name_selector_dialog)
{
	GtkTreeSelection *contact_selection;
	gboolean          have_selection = FALSE;
	gint              i;

	contact_selection = gtk_tree_view_get_selection (name_selector_dialog->contact_view);
	if (gtk_tree_selection_count_selected_rows (contact_selection))
		have_selection = TRUE;

	for (i = 0; i < name_selector_dialog->sections->len; i++) {
		Section *section = &g_array_index (name_selector_dialog->sections, Section, i);
		gtk_widget_set_sensitive (GTK_WIDGET (section->transfer_button), have_selection);
	}
}

static void
contact_activated (ENameSelectorDialog *name_selector_dialog, GtkTreePath *path)
{
	EContactStore     *contact_store;
	EDestinationStore *destination_store;
	EContact          *contact;
	Section           *section;
	gint               email_n;
	GList		  *rows, *l;
	GtkTreeSelection  *contact_selection;

	/* When a contact is activated, we transfer it to the first destination on our list */

	contact_store = e_name_selector_model_peek_contact_store (name_selector_dialog->name_selector_model);

	/* If we have no sections, we can't transfer */
	if (name_selector_dialog->sections->len == 0) 
		return;

	section = &g_array_index (name_selector_dialog->sections, Section, 0);
	if (!e_name_selector_model_peek_section (name_selector_dialog->name_selector_model,
						 section->name, NULL, &destination_store)) {
		g_warning ("ENameSelectorDialog has a section unknown to the model!");
		return;
	}

	contact_selection = gtk_tree_view_get_selection (name_selector_dialog->contact_view);
	rows = gtk_tree_selection_get_selected_rows (contact_selection, NULL);
	rows = g_list_reverse (rows);

	/* Get the contacts to be transferred */

	for (l=rows; l; l=g_list_next(l)) {
		GtkTreeIter iter;

		if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (name_selector_dialog->contact_sort),
				      &iter, path))
			g_assert_not_reached ();

		gtk_tree_path_free (path);
		sort_iter_to_contact_store_iter (name_selector_dialog, &iter, &email_n);
		
		contact = e_contact_store_get_contact (contact_store, &iter);
		if (!contact) {
			g_warning ("ENameSelectorDialog could not get selected contact!");
			g_list_free (rows);
			return;
		}
		add_destination (destination_store, contact, email_n);
	}
	g_list_free (rows);
}

static void
destination_activated (ENameSelectorDialog *name_selector_dialog, GtkTreePath *path,
		       GtkTreeViewColumn *column, GtkTreeView *tree_view)
{
	gint               section_index;
	EDestinationStore *destination_store;
	EDestination      *destination;
	Section           *section;
	GList		  *rows, *l;
	GtkTreeSelection  *selection;

	/* When a destination is activated, we remove it from the section */

	section_index = find_section_by_tree_view (name_selector_dialog, tree_view);
	if (section_index < 0) {
		g_warning ("ENameSelectorDialog got activation from unknown view!");
		return;
	}

	section = &g_array_index (name_selector_dialog->sections, Section, section_index);
	if (!e_name_selector_model_peek_section (name_selector_dialog->name_selector_model,
						 section->name, NULL, &destination_store)) {
		g_warning ("ENameSelectorDialog has a section unknown to the model!");
		return;
	}

	rows = gtk_tree_selection_get_selected_rows (selection, NULL);
	rows = g_list_reverse (rows);

	for (l = rows; l; l = g_list_next(l)) {
		GtkTreeIter iter;
		GtkTreePath *path = l->data;

		if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (destination_store),
				      	      &iter, path))
			g_assert_not_reached ();

		gtk_tree_path_free (path);

		destination = e_destination_store_get_destination (destination_store, &iter);
		g_assert (destination);

		e_destination_store_remove_destination (destination_store, destination);
	}
	g_list_free (rows);
}

static gboolean 
remove_selection (ENameSelectorDialog *name_selector_dialog, GtkTreeView *tree_view)
{
	gint               section_index;
	EDestinationStore *destination_store;
	EDestination      *destination;
	Section           *section;
	GtkTreeSelection  *selection;
	GList		  *rows, *l;

	section_index = find_section_by_tree_view (name_selector_dialog, tree_view);
	if (section_index < 0) {
		g_warning ("ENameSelectorDialog got key press from unknown view!");
		return FALSE;
	}

	section = &g_array_index (name_selector_dialog->sections, Section, section_index);
	if (!e_name_selector_model_peek_section (name_selector_dialog->name_selector_model,
						 section->name, NULL, &destination_store)) {
		g_warning ("ENameSelectorDialog has a section unknown to the model!");
		return FALSE;
	}

	selection = gtk_tree_view_get_selection (tree_view);
	if (!gtk_tree_selection_count_selected_rows (selection)) {
		g_warning ("ENameSelectorDialog remove button clicked, but no selection!");
		return FALSE;
	}

	rows = gtk_tree_selection_get_selected_rows (selection, NULL);
	rows = g_list_reverse (rows);

	for (l = rows; l; l = g_list_next(l)) {
		GtkTreeIter iter;
		GtkTreePath *path = l->data;

		if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (destination_store),
					      &iter, path))
			g_assert_not_reached ();

		gtk_tree_path_free (path);

		destination = e_destination_store_get_destination (destination_store, &iter);
		g_assert (destination);

		e_destination_store_remove_destination (destination_store, destination);
	}
	g_list_free (rows);

	return TRUE;
}

static void
remove_button_clicked (GtkButton *button, SelData *data)
{
	GtkTreeView *view;
	ENameSelectorDialog *name_selector_dialog;
	
	view = data->view; 
	name_selector_dialog = data->dlg_ptr;
	remove_selection (name_selector_dialog, view);
}
	
static gboolean 
destination_key_press (ENameSelectorDialog *name_selector_dialog, 
		       GdkEventKey *event, GtkTreeView *tree_view)
{

	/* we only care about DEL key */
	if (event->keyval != GDK_Delete)
		return FALSE;
	return remove_selection (name_selector_dialog, tree_view);

}

static void
transfer_button_clicked (ENameSelectorDialog *name_selector_dialog, GtkButton *transfer_button)
{
	EContactStore     *contact_store;
	EDestinationStore *destination_store;
	GtkTreeSelection  *selection;
	EContact          *contact;
	gint               section_index;
	Section           *section;
	gint               email_n;
	GList		  *rows, *l;

	/* Get the contact to be transferred */

	contact_store = e_name_selector_model_peek_contact_store (name_selector_dialog->name_selector_model);
	selection = gtk_tree_view_get_selection (name_selector_dialog->contact_view);

	if (!gtk_tree_selection_count_selected_rows (selection)) {
		g_warning ("ENameSelectorDialog transfer button clicked, but no selection!");
		return;
	}

	/* Get the target section */
	section_index = find_section_by_transfer_button (name_selector_dialog, transfer_button);
	if (section_index < 0) {
		g_warning ("ENameSelectorDialog got click from unknown button!");
		return;
	}

	section = &g_array_index (name_selector_dialog->sections, Section, section_index);
	if (!e_name_selector_model_peek_section (name_selector_dialog->name_selector_model,
						 section->name, NULL, &destination_store)) {
		g_warning ("ENameSelectorDialog has a section unknown to the model!");
		return;
	}

	rows = gtk_tree_selection_get_selected_rows (selection, NULL);
	rows = g_list_reverse (rows);

	for (l = rows; l; l = g_list_next(l)) {
		GtkTreeIter iter;
		GtkTreePath *path = l->data;

		if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (name_selector_dialog->contact_sort),
				      &iter, path))
			g_assert_not_reached ();

		gtk_tree_path_free (path);
		sort_iter_to_contact_store_iter (name_selector_dialog, &iter, &email_n);

		contact = e_contact_store_get_contact (contact_store, &iter);
		if (!contact) {
			g_warning ("ENameSelectorDialog could not get selected contact!");
			g_list_free (rows);
			return;
		}

		add_destination (destination_store, contact, email_n);
	}
	g_list_free (rows);
}

/* --------------------- *
 * Main model management *
 * --------------------- */

static void
setup_name_selector_model (ENameSelectorDialog *name_selector_dialog)
{
	EContactStore       *contact_store;
	ETreeModelGenerator *contact_filter;
	GList               *new_sections;
	GList               *l;
	gint                 i;

	/* Rid UI of previous destination sections */

	for (i = 0; i < name_selector_dialog->sections->len; i++)
		free_section (name_selector_dialog, i);

	g_array_set_size (name_selector_dialog->sections, 0);

	/* Create new destination sections in UI */

	new_sections = e_name_selector_model_list_sections (name_selector_dialog->name_selector_model);

	for (l = new_sections; l; l = g_list_next (l)) {
		gchar             *name = l->data;
		gchar             *pretty_name;
		EDestinationStore *destination_store;

		e_name_selector_model_peek_section (name_selector_dialog->name_selector_model,
						    name, &pretty_name, &destination_store);

		add_section (name_selector_dialog, name, pretty_name, destination_store);

		g_free (pretty_name);
		g_free (name);
	}

	g_list_free (new_sections);

	/* Connect to section add/remove signals */

	g_signal_connect_swapped (name_selector_dialog->name_selector_model, "section-added",
				  G_CALLBACK (model_section_added), name_selector_dialog);
	g_signal_connect_swapped (name_selector_dialog->name_selector_model, "section-removed",
				  G_CALLBACK (model_section_removed), name_selector_dialog);

	/* Get contact store and its filter wrapper */

	contact_store  = e_name_selector_model_peek_contact_store  (name_selector_dialog->name_selector_model);
	contact_filter = e_name_selector_model_peek_contact_filter (name_selector_dialog->name_selector_model);

	/* Create sorting model on top of filter, assign it to view */

	if (name_selector_dialog->contact_sort)
		g_object_unref (name_selector_dialog->contact_sort);

	name_selector_dialog->contact_sort = GTK_TREE_MODEL_SORT (
		gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (contact_filter)));

	/* sort on full name as we display full name in name selector dialog */
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (name_selector_dialog->contact_sort),
					      E_CONTACT_FULL_NAME, GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (name_selector_dialog->contact_view,
				 GTK_TREE_MODEL (name_selector_dialog->contact_sort));

	/* Make sure UI is consistent */

	search_changed (name_selector_dialog);
	contact_selection_changed (name_selector_dialog);
}

static void
deep_free_list (GList *list)
{
	GList *l;

	for (l = list; l; l = g_list_next (l))
		g_free (l->data);

	g_list_free (list);
}

static void
contact_column_formatter (GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model,
			  GtkTreeIter *iter, ENameSelectorDialog *name_selector_dialog)
{
	EContactStore *contact_store;
	EContact      *contact;
	GtkTreeIter    contact_store_iter;
	GList         *email_list;
	gchar         *string;
	gchar         *full_name_str;
	gchar         *email_str;
	gint           email_n;

	contact_store_iter = *iter;
	sort_iter_to_contact_store_iter (name_selector_dialog, &contact_store_iter, &email_n);

	contact_store = e_name_selector_model_peek_contact_store (name_selector_dialog->name_selector_model);
	contact = e_contact_store_get_contact (contact_store, &contact_store_iter);
	email_list = e_contact_get (contact, E_CONTACT_EMAIL);
	email_str = g_list_nth_data (email_list, email_n);
	full_name_str = e_contact_get (contact, E_CONTACT_FULL_NAME);

	if (e_contact_get (contact, E_CONTACT_IS_LIST)) {
		string = g_strdup_printf ("%s", full_name_str ? full_name_str : "?");
	} else {
		string = g_strdup_printf ("%s%s<%s>", full_name_str ? full_name_str : "",
					  full_name_str ? " " : "",
					  email_str ? email_str : "");
	}

	g_free (full_name_str);
	deep_free_list (email_list);

	g_object_set (cell, "text", string, NULL);
	g_free (string);
}

static void
destination_column_formatter (GtkTreeViewColumn *column, GtkCellRenderer *cell, GtkTreeModel *model,
			      GtkTreeIter *iter, ENameSelectorDialog *name_selector_dialog)
{
	EDestinationStore *destination_store = E_DESTINATION_STORE (model);
	EDestination      *destination;
	gchar             *string;

       	destination = e_destination_store_get_destination (destination_store, iter);
	g_assert (destination);

	if (e_destination_is_evolution_list (destination)) {
		if (e_destination_list_show_addresses (destination)) {
			const gchar *name;
			const gchar *addresses;

			name      = e_destination_get_name (destination);
			addresses = e_destination_get_address (destination);

			string = g_strdup_printf ("%s%s(%s)", name ? name : "",
						  name ? " " : "", addresses ? addresses : "?");
		} else {
			string = g_strdup (e_destination_get_name (destination));
		}
	} else {
		string = g_strdup (e_destination_get_address (destination));
	}

	g_object_set (cell, "text", string, NULL);
	g_free (string);
}

/* ----------------------- *
 * ENameSelectorDialog API *
 * ----------------------- */

/**
 * e_name_selector_dialog_peek_model:
 * @name_selector_dialog: an #ENameSelectorDialog
 *
 * Gets the #ENameSelectorModel used by @name_selector_model.
 *
 * Return value: The #ENameSelectorModel being used.
 **/
ENameSelectorModel *
e_name_selector_dialog_peek_model (ENameSelectorDialog *name_selector_dialog)
{
	g_return_val_if_fail (E_IS_NAME_SELECTOR_DIALOG (name_selector_dialog), NULL);

	return name_selector_dialog->name_selector_model;
}

/**
 * e_name_selector_dialog_set_model:
 * @name_selector_dialog: an #ENameSelectorDialog
 * @model: an #ENameSelectorModel
 *
 * Sets the model being used by @name_selector_dialog to @model.
 **/
void
e_name_selector_dialog_set_model (ENameSelectorDialog *name_selector_dialog,
				  ENameSelectorModel  *model)
{
	g_return_if_fail (E_IS_NAME_SELECTOR_DIALOG (name_selector_dialog));
	g_return_if_fail (E_IS_NAME_SELECTOR_MODEL (model));

	if (model == name_selector_dialog->name_selector_model)
		return;

	g_signal_handlers_disconnect_matched (name_selector_dialog->name_selector_model,
					      G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, name_selector_dialog);

	g_object_unref (name_selector_dialog->name_selector_model);
	name_selector_dialog->name_selector_model = g_object_ref (model);

	setup_name_selector_model (name_selector_dialog);
}

/* ----------------------------------- *
 * Widget creation functions for Glade *
 * ----------------------------------- */

#if CATEGORIES_COMPONENTS_MOVED

GtkWidget *
e_name_selector_dialog_create_categories (void)
{
	ECategoriesMasterList *ecml;
	GtkWidget             *option_menu;

	ecml = e_categories_master_list_wombat_new ();
	option_menu = e_categories_master_list_option_menu_new (ecml);
	g_object_unref (ecml);

	return option_menu;
}

#endif
