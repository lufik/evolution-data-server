/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@novell.com>
 *
 *  Copyright 2005 Novell, Inc. (www.novell.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef __CAMEL_OFFLINE_FOLDER_H__
#define __CAMEL_OFFLINE_FOLDER_H__

#include <camel/camel-folder.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define CAMEL_OFFLINE_FOLDER_TYPE     (camel_offline_folder_get_type ())
#define CAMEL_OFFLINE_FOLDER(obj)     (CAMEL_CHECK_CAST((obj), CAMEL_OFFLINE_FOLDER_TYPE, CamelOfflineFolder))
#define CAMEL_OFFLINE_FOLDER_CLASS(k) (CAMEL_CHECK_CLASS_CAST ((k), CAMEL_OFFLINE_FOLDER_TYPE, CamelOfflineFolderClass))
#define CAMEL_IS_OFFLINE_FOLDER(o)    (CAMEL_CHECK_TYPE((o), CAMEL_OFFLINE_FOLDER_TYPE))

typedef struct _CamelOfflineFolder CamelOfflineFolder;
typedef struct _CamelOfflineFolderClass CamelOfflineFolderClass;

enum {
	CAMEL_OFFLINE_FOLDER_ARG_SYNC_OFFLINE = CAMEL_FOLDER_ARG_LAST,
	CAMEL_OFFLINE_FOLDER_ARG_LAST = CAMEL_FOLDER_ARG_LAST + 0x100
};

enum {
	CAMEL_OFFLINE_FOLDER_SYNC_OFFLINE = CAMEL_OFFLINE_FOLDER_ARG_SYNC_OFFLINE | CAMEL_ARG_BOO,
};

struct _CamelOfflineFolder {
	CamelFolder parent_object;
	
	unsigned int sync_offline:1;
};

struct _CamelOfflineFolderClass {
	CamelFolderClass parent_class;
	
	void (* downsync) (CamelOfflineFolder *folder, const char *expression, CamelException *ex);
};


CamelType camel_offline_folder_get_type (void);

void camel_offline_folder_downsync (CamelOfflineFolder *folder, const char *expression, CamelException *ex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CAMEL_OFFLINE_FOLDER_H__ */
