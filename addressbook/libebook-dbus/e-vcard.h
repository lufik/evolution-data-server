/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-vcard.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU Lesser General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Chris Toshok (toshok@ximian.com)
 */

#ifndef _EVCARD_H
#define _EVCARD_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION: e-vcard
 * @title: EVCard
 * @short_description: Contact vCard representation
 * @see_also: #EContact
 *
 * #EVCard is an object representation of a contact's vCard and includes
 * extensive support for manipulating vCards and their attributes and their
 * parameters.
 *
 * Typically, you will only use #EContact contacts, and not "plain" #EVCard
 * objects.
 *
 * Example for adding an XMPP username to a contact:
 *
 * |[
 * void
 * add_xmpp_username (EContact *contact, const char *username)
 * {
 *      EVCardAttribute *attr;
 *      EVCardAttributeParam *param;
 *
 *      attr = e_vcard_attribute_new (NULL, EVC_X_JABBER);
 *
 *      /<!-- -->* Optionally specifying the real-life context for this username
 *      ("work"); see RFC 2426 for valid values *<!-- -->/
 *      param = e_vcard_attribute_param_new (EVC_TYPE);
 *      e_vcard_attribute_add_param_with_value (attr, param, "WORK");
 *
 *      e_vcard_add_attribute_with_value (E_VCARD (contact), attr, username);
 * }
 * ]|
 */

/**
 * EVC_ADR:
 *
 * Postal/mailing address
 */
#define EVC_ADR             "ADR"
/**
 * EVC_BDAY:
 *
 * Birthdate in ISO 8601 date or combined-date-and-time (in UTC or
 * UTC-with-offset) format.
 */
#define EVC_BDAY            "BDAY"
#define EVC_CALURI          "CALURI"
#define EVC_CATEGORIES      "CATEGORIES"
/**
 * EVC_EMAIL:
 *
 * Email address
 */
#define EVC_EMAIL           "EMAIL"
/**
 * EVC_ENCODING:
 *
 * The encoding format of the vCard
 */
#define EVC_ENCODING        "ENCODING"
#define EVC_FBURL           "FBURL"
/**
 * EVC_FN:
 *
 * Full name; this is a single-string representation of the N field.
 */
#define EVC_FN              "FN"
/**
 * EVC_GEO:
 *
 * Geographical location field
 */
#define EVC_GEO		    "GEO"
#define EVC_ICSCALENDAR     "ICSCALENDAR" /* XXX should this be X-EVOLUTION-ICSCALENDAR? */
#define EVC_KEY             "KEY"
#define EVC_LABEL           "LABEL"
#define EVC_LOGO            "LOGO"
#define EVC_MAILER          "MAILER"
/**
 * EVC_NICKNAME:
 *
 * A contact's nickname
 */
#define EVC_NICKNAME        "NICKNAME"
/**
 * EVC_N:
 *
 * Structured name field. The values for this field are, in order, as follows:
 * &lt;family name&gt;;&lt;given name&gt;;&lt;middle name(s)&gt;;&lt;honorific prefix(es()&gt;;&lt;honorific suffix(es)&gt;
 *
 * Example from RFC 2426:
 *    N:Stevenson;John;Philip,Paul;Dr.;Jr.,M.D.,A.C.P.
 */
#define EVC_N               "N"
/**
 * EVC_NOTE:
 *
 * An arbitrary user-set note about the contact
 */
#define EVC_NOTE            "NOTE"
/**
 * EVC_ORG:
 *
 * Organization/Company
 */
#define EVC_ORG             "ORG"
/**
 * EVC_PHOTO:
 *
 * Structured Photo/avatar. See RFC 2426 for more details.
 */
#define EVC_PHOTO           "PHOTO"
#define EVC_PRODID          "PRODID"
#define EVC_QUOTEDPRINTABLE "QUOTED-PRINTABLE"
#define EVC_REV             "REV"
#define EVC_ROLE            "ROLE"
/**
 * EVC_TEL:
 *
 * Telephone or fax number.
 */
#define EVC_TEL             "TEL"
/**
 * EVC_TITLE:
 *
 * Professional title (e.g., "Director, Standards and Practices Dept.")
 */
#define EVC_TITLE           "TITLE"
/**
 * EVC_TYPE:
 *
 * The vCard "TYPE" attribute parameter. Usually omitted, "HOME", "WORK", or
 * "OTHER".
 */
#define EVC_TYPE            "TYPE"
/**
 * EVC_UID:
 *
 * Unique ID
 */
#define EVC_UID             "UID"
/**
 * EVC_URL:
 *
 * A Web URL
 */
#define EVC_URL             "URL"
#define EVC_VALUE           "VALUE"
#define EVC_VERSION         "VERSION"

/**
 * EVC_X_AIM:
 *
 * An AOL Instant Messenger username
 */
#define EVC_X_AIM              "X-AIM"
#define EVC_X_ANNIVERSARY      "X-EVOLUTION-ANNIVERSARY"
#define EVC_X_ASSISTANT        "X-EVOLUTION-ASSISTANT"
#define EVC_X_BIRTHDAY         "X-EVOLUTION-BIRTHDAY"
#define EVC_X_BLOG_URL         "X-EVOLUTION-BLOG-URL"
#define EVC_X_CALLBACK         "X-EVOLUTION-CALLBACK"
#define EVC_X_COMPANY          "X-EVOLUTION-COMPANY"
#define EVC_X_DEST_CONTACT_UID "X-EVOLUTION-DEST-CONTACT-UID"
#define EVC_X_DEST_EMAIL       "X-EVOLUTION-DEST-EMAIL"
#define EVC_X_DEST_EMAIL_NUM   "X-EVOLUTION-DEST-EMAIL-NUM"
#define EVC_X_DEST_HTML_MAIL   "X-EVOLUTION-DEST-HTML-MAIL"
#define EVC_X_DEST_NAME        "X-EVOLUTION-DEST-NAME"
#define EVC_X_DEST_SOURCE_UID  "X-EVOLUTION-DEST-SOURCE-UID"
#define EVC_X_FILE_AS          "X-EVOLUTION-FILE-AS"
/**
 * EVC_X_ICQ:
 *
 * An ICQ messenger username
 */
#define EVC_X_ICQ              "X-ICQ"
/**
 * EVC_X_JABBER:
 *
 * A Jabber/XMPP username
 */
#define EVC_X_JABBER           "X-JABBER"
#define EVC_X_LIST_SHOW_ADDRESSES "X-EVOLUTION-LIST-SHOW_ADDRESSES"
#define EVC_X_LIST          	"X-EVOLUTION-LIST"
#define EVC_X_MANAGER       	"X-EVOLUTION-MANAGER"
/**
 * EVC_X_MSN:
 *
 * A Microsoft Network/MSN Messenger username
 */
#define EVC_X_MSN           	"X-MSN"
#define EVC_X_RADIO         	"X-EVOLUTION-RADIO"
/**
 * EVC_X_SIP:
 *
 * A SIP VoIP username
 */
#define EVC_X_SIP           	"X-SIP"
#define EVC_X_SPOUSE        	"X-EVOLUTION-SPOUSE"
#define EVC_X_TELEX         	"X-EVOLUTION-TELEX"
#define EVC_X_TTYTDD        	"X-EVOLUTION-TTYTDD"
#define EVC_X_VIDEO_URL     	"X-EVOLUTION-VIDEO-URL"
#define EVC_X_WANTS_HTML    	"X-MOZILLA-HTML"
/**
 * EVC_X_YAHOO:
 *
 * A Yahoo! Messenger username
 */
#define EVC_X_YAHOO         	"X-YAHOO"
/**
 * EVC_X_GADUGADU:
 *
 * A Gadu Gadu messenger username
 */
#define EVC_X_GADUGADU        "X-GADUGADU"
/**
 * EVC_X_GROUPWISE:
 *
 * A Novell Groupwise username
 */
#define EVC_X_GROUPWISE     	"X-GROUPWISE"
/**
 * EVC_X_BOOK_URI:
 *
 * The URI of the contact's #EBook store. Do not set this manually.
 */
#define EVC_X_BOOK_URI     	"X-EVOLUTION-BOOK-URI"
/**
 * EVC_X_SKYPE:
 *
 * A Skype username
 */
#define EVC_X_SKYPE           "X-SKYPE"

/**
 * EVCardFormat:
 * @EVC_FORMAT_VCARD_21: the vCard is formatted for version 2.1 of the
 * specification
 * @EVC_FORMAT_VCARD_30: the vCard is formatted for version 3.0 of the
 * specification (RFC 2426)
 *
 * The specific vCard format version that the vCard complies with.
 **/
typedef enum {
	EVC_FORMAT_VCARD_21,
	EVC_FORMAT_VCARD_30
} EVCardFormat;

#define E_TYPE_VCARD            (e_vcard_get_type ())
#define E_VCARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), E_TYPE_VCARD, EVCard))
#define E_VCARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), E_TYPE_VCARD, EVCardClass))
#define E_IS_VCARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), E_TYPE_VCARD))
#define E_IS_VCARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), E_TYPE_VCARD))
#define E_VCARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), E_TYPE_VCARD, EVCardClass))

#define E_TYPE_VCARD_ATTRIBUTE  (e_vcard_attribute_get_type ())

typedef struct _EVCard EVCard;
typedef struct _EVCardClass EVCardClass;
typedef struct _EVCardPrivate EVCardPrivate;

/** 
 * EVCardAttribute:
 *
 * All the fields of this structure are private to the object's implementation
 * and should never be accessed directly.
 **/
typedef struct _EVCardAttribute EVCardAttribute;

/** 
 * EVCardAttributeParam:
 *
 * All the fields of this structure are private to the object's implementation
 * and should never be accessed directly.
 **/
typedef struct _EVCardAttributeParam EVCardAttributeParam;

/**
 * EVCard:
 *
 * All the fields of this structure are private to the object's implementation
 * and should never be accessed directly.
 **/
struct _EVCard {
	GObject parent;
	/*< private >*/
	EVCardPrivate *priv;
};

struct _EVCardClass {
	GObjectClass parent_class;

	/* Padding for future expansion */
	void (*add_attribute)     (EVCard *evc, EVCardAttribute *attr);
	void (*remove_attribute)  (EVCard *evc, EVCardAttribute *attr);
	void (*_ebook_reserved2)  (void);
	void (*_ebook_reserved3)  (void);
	void (*_ebook_reserved4)  (void);
};

GType   e_vcard_get_type                     (void) G_GNUC_CONST;

gboolean e_vcard_construct                   (EVCard *evc, const char *str);
gboolean e_vcard_construct_with_uid          (EVCard *evc, const char *str, const char *uid);

EVCard* e_vcard_new                          (void);
EVCard* e_vcard_new_from_string              (const char *str);

char*   e_vcard_to_string                    (EVCard *evc, EVCardFormat format);

/* mostly for debugging */
void     e_vcard_dump_structure              (EVCard *evc);
gboolean e_vcard_is_parsed                   (EVCard *evc);

/* mostly for derived classes */
gboolean e_vcard_is_parsing                  (EVCard *evc);


/* attributes */
GType            e_vcard_attribute_get_type          (void) G_GNUC_CONST;
EVCardAttribute *e_vcard_attribute_new               (const char *attr_group, const char *attr_name);
void             e_vcard_attribute_free              (EVCardAttribute *attr);
EVCardAttribute *e_vcard_attribute_copy              (EVCardAttribute *attr);
gboolean         e_vcard_attribute_equal             (EVCardAttribute *attr_a, EVCardAttribute *attr_b);

void             e_vcard_remove_attributes           (EVCard *evc, const char *attr_group, const char *attr_name);
void             e_vcard_remove_attribute            (EVCard *evc, EVCardAttribute *attr);
void             e_vcard_add_attribute               (EVCard *evc, EVCardAttribute *attr);
void             e_vcard_add_attribute_with_value    (EVCard *evcard, EVCardAttribute *attr, const char *value);
void             e_vcard_add_attribute_with_values   (EVCard *evcard, EVCardAttribute *attr, ...);
void             e_vcard_attribute_add_value         (EVCardAttribute *attr, const char *value);
void             e_vcard_attribute_add_value_decoded (EVCardAttribute *attr, const char *value, int len);
void             e_vcard_attribute_add_values        (EVCardAttribute *attr, ...);
void             e_vcard_attribute_remove_value      (EVCardAttribute *attr, const char *s);
void             e_vcard_attribute_remove_values     (EVCardAttribute *attr);
void             e_vcard_attribute_remove_params     (EVCardAttribute *attr);
void             e_vcard_attribute_remove_param      (EVCardAttribute *attr, const char *param_name);
void             e_vcard_attribute_remove_param_value (EVCardAttribute *attr, const char *param_name, const char *s);

/* attribute parameters */
EVCardAttributeParam* e_vcard_attribute_param_new             (const char *name);
void                  e_vcard_attribute_param_free            (EVCardAttributeParam *param);
EVCardAttributeParam* e_vcard_attribute_param_copy            (EVCardAttributeParam *param);
void                  e_vcard_attribute_add_param             (EVCardAttribute *attr, EVCardAttributeParam *param);
void                  e_vcard_attribute_add_param_with_value  (EVCardAttribute *attr,
							       EVCardAttributeParam *param, const char *value);
void                  e_vcard_attribute_add_param_with_values (EVCardAttribute *attr,
							       EVCardAttributeParam *param, ...);

void                  e_vcard_attribute_param_add_value       (EVCardAttributeParam *param,
							       const char *value);
void                  e_vcard_attribute_param_add_values      (EVCardAttributeParam *param,
							       ...);
void                  e_vcard_attribute_param_remove_values   (EVCardAttributeParam *param);

void                  e_vcard_attribute_merge_param             (EVCardAttribute *attr, EVCardAttributeParam *param,
								 GCompareFunc cmp_func);

void                  e_vcard_attribute_merge_param_with_value  (EVCardAttribute *attr, EVCardAttributeParam *param,
								 const char *value, GCompareFunc cmp_func);
void                  e_vcard_attribute_merge_param_with_values (EVCardAttribute *attr, EVCardAttributeParam *param,
								 GCompareFunc cmp_func, ...);

void                  e_vcard_attribute_param_merge_value       (EVCardAttributeParam *param, const char *value,
								 GCompareFunc cmp_func);
void                  e_vcard_attribute_param_merge_values      (EVCardAttributeParam *param, GCompareFunc cmp_func,
								 ...);

/* EVCard* accessors.  nothing returned from these functions should be
   freed by the caller. */
EVCardAttribute *e_vcard_get_attribute        (EVCard *evc, const char *name);
GList*           e_vcard_get_attributes       (EVCard *evcard);
const char*      e_vcard_attribute_get_group  (EVCardAttribute *attr);
const char*      e_vcard_attribute_get_name   (EVCardAttribute *attr);
GList*           e_vcard_attribute_get_values (EVCardAttribute *attr);  /* GList elements are of type char* */
GList*           e_vcard_attribute_get_values_decoded (EVCardAttribute *attr); /* GList elements are of type GString* */

/* special accessors for single valued attributes */
gboolean              e_vcard_attribute_is_single_valued      (EVCardAttribute *attr);
char*                 e_vcard_attribute_get_value             (EVCardAttribute *attr);
GString*              e_vcard_attribute_get_value_decoded     (EVCardAttribute *attr);

GList*           e_vcard_attribute_get_params       (EVCardAttribute *attr);
GList*           e_vcard_attribute_get_param        (EVCardAttribute *attr, const char *name);
const char*      e_vcard_attribute_param_get_name   (EVCardAttributeParam *param);
GList*           e_vcard_attribute_param_get_values (EVCardAttributeParam *param);

/* special TYPE= parameter predicate (checks for TYPE=@typestr */
gboolean         e_vcard_attribute_has_type         (EVCardAttribute *attr, const char *typestr);

/* Utility functions. */
char*            e_vcard_escape_string (const char *s);
char*            e_vcard_unescape_string (const char *s);

GList*           e_vcard_util_split_cards (const char *str, gsize *len);

G_END_DECLS

#endif /* _EVCARD_H */
