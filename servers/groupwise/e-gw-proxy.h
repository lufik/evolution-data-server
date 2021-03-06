/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Authors : 
 *  Shreyas Srinivasan <sshreyas@novell.com>
 *  Sankar P <psankar@novell.com>
 *
 * Copyright 2003, Novell, Inc.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of version 2 of the GNU Lesser General Public 
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libsoup/soup-soap-message.h>

/*State of each proxy grant*/

#define E_GW_PROXY_NEW           (1 << 0)           
#define E_GW_PROXY_DELETED       (1 << 1)
#define E_GW_PROXY_EDITED	    (1 << 2)

/*Permissions associated with a proxy grant*/
#define E_GW_PROXY_MAIL_READ                (1 << 0)           
#define E_GW_PROXY_MAIL_WRITE               (1 << 1)
#define E_GW_PROXY_APPOINTMENT_READ         (1 << 2)           
#define E_GW_PROXY_APPOINTMENT_WRITE        (1 << 3)
#define E_GW_PROXY_NOTES_READ               (1 << 4)
#define E_GW_PROXY_NOTES_WRITE              (1 << 5)
#define E_GW_PROXY_TASK_READ                (1 << 6)
#define E_GW_PROXY_TASK_WRITE               (1 << 7)
#define E_GW_PROXY_GET_ALARMS               (1 << 8)
#define E_GW_PROXY_GET_NOTIFICATIONS        (1 << 9)
#define E_GW_PROXY_MODIFY_FOLDERS           (1 << 10)
#define E_GW_PROXY_READ_PRIVATE             (1 << 11)


struct _proxyHandler {
    char *uniqueid;
    char *proxy_name;
    char *proxy_email;   
    guint32 flags;
    guint32 permissions;
};

typedef struct _proxyHandler proxyHandler;

void e_gw_proxy_construct_proxy_access_list (SoupSoapParameter *param, GList **proxy_list);
void e_gw_proxy_construct_proxy_list (SoupSoapParameter *param, GList **proxy_info);
void e_gw_proxy_form_proxy_add_msg (SoupSoapMessage *msg, proxyHandler *new_proxy);
void e_gw_proxy_form_proxy_remove_msg (SoupSoapMessage *msg, proxyHandler *removeProxy);
void e_gw_proxy_form_modify_proxy_msg (SoupSoapMessage *msg, proxyHandler *new_proxy);
void e_gw_proxy_parse_proxy_login_response (SoupSoapParameter *param, int *permissions);

