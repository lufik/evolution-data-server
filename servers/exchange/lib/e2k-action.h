/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* Copyright (C) 2002-2004 Novell, Inc. */

#ifndef __E2K_ACTION_H__
#define __E2K_ACTION_H__

#include "e2k-types.h"
#include "e2k-properties.h"
#include "e2k-rule.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

gboolean     e2k_actions_extract       (guint8        	    **data,
					int           	     *len,
					GPtrArray     	    **actions);
void         e2k_actions_append        (GByteArray    	     *ba,
					GPtrArray     	     *actions);

E2kAction   *e2k_action_move           (GByteArray    	     *store_entryid,
					GByteArray    	     *folder_source_key);
E2kAction   *e2k_action_copy           (GByteArray    	     *store_entryid,
					GByteArray    	     *folder_source_key);
E2kAction   *e2k_action_reply          (GByteArray    	     *template_entryid,
					guint8    	      template_guid[16]);
E2kAction   *e2k_action_oof_reply      (GByteArray    	     *template_entryid,
					guint8    	      template_guid[16]);
E2kAction   *e2k_action_defer          (GByteArray           *data);
E2kAction   *e2k_action_bounce         (E2kActionBounceCode   bounce_code);
E2kAction   *e2k_action_forward        (E2kAddrList    	     *list);
E2kAction   *e2k_action_delegate       (E2kAddrList    	     *list);
E2kAction   *e2k_action_tag            (const char     	     *propname,
					E2kPropType    	      type,
					gpointer       	      value);
E2kAction   *e2k_action_delete         (void);

void         e2k_actions_free          (GPtrArray      	     *actions);
void         e2k_action_free           (E2kAction            *act);


E2kAddrList *e2k_addr_list_new         (int            	      nentries);
void         e2k_addr_list_set_local   (E2kAddrList    	     *list,
					int            	      entry_num,
					const char     	     *display_name,
					const char     	     *exchange_dn,
					const char     	     *email);
void         e2k_addr_list_set_oneoff  (E2kAddrList    	     *list,
					int            	      entry_num,
					const char     	     *display_name,
					const char     	     *email);
void         e2k_addr_list_free        (E2kAddrList    	     *list);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __E2K_ACTION_H__ */
