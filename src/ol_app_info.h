/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011 Tiger Soldier <tigersoldier@gmail.com>
 *
 * This file is part of OSD Lyrics.
 * 
 * OSD Lyrics is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OSD Lyrics is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OSD Lyrics.  If not, see <https://www.gnu.org/licenses/>. 
 */
#ifndef _OL_APP_INFO_H_
#define _OL_APP_INFO_H_

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define OL_TYPE_APP_INFO         (ol_app_info_get_type ())
#define OL_APP_INFO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), OL_TYPE_APP_INFO, OlAppInfo))
#define OL_APP_INFO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), OL_TYPE_APP_INFO, OlAppInfoClass))
#define OL_IS_APP_INFO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), OL_TYPE_APP_INFO))
#define OL_IS_APP_INFO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), OL_TYPE_APP_INFO))
#define OL_APP_INFO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), OL_TYPE_APP_INFO, OlAppInfoClass))

enum OlAppInfoFlags {
  OL_APP_INFO_FLAGS_NONE          = 0,
  /// Find the first app info with prefix of given commandline.
  OL_APP_INFO_WITH_PREFIX         = 1 << 0,
  /// The first argument is the real program name. Uses in the command
  /// with sudo/gksu
  OL_APP_INFO_SECOND_IS_EXEC      = 1 << 1,
  /// Look up desktop file for application name.
  OL_APP_INFO_USE_DESKTOP_NAME    = 1 << 2,
  /// Look up desktop file for application icon.
  OL_APP_INFO_USE_DESKTOP_ICON    = 1 << 3,
  /// Look up desktop file for application command line.
  OL_APP_INFO_USE_DESKTOP_CMDLINE = 1 << 4,
  /// Use the information in desktop file rather than the parameters, if available.
  OL_APP_INFO_PREFER_DESKTOP_FILE = OL_APP_INFO_USE_DESKTOP_NAME |
  OL_APP_INFO_USE_DESKTOP_ICON | OL_APP_INFO_USE_DESKTOP_CMDLINE,
};

typedef struct _OlAppInfo OlAppInfo;
typedef struct _OlAppInfoClass
{
  GObjectClass parent_class;
} OlAppInfoClass;

/**
 * Creates an GAppInfo according to cmdline.
 *
 * If #name is not #NULL, use the command name of #cmdline as the name.
 *
 * If #icon_name is #NULL, use the command name of #cmdline as the icon name.
 *
 * If #OL_APP_INFO_WITH_PREFIX is set in #flags, and the command specified in
 * #cmdline not exist, the command with the prefix of the command name, if any,
 * is used.
 *
 * If the #cmdline is started with gksu or somethine like that, you can use
 * #OL_APP_INFO_SECOND_IS_EXEC, so the first argument (the second) in the #cmdline
 * is use as the command name.
 *
 * If any of #OL_APP_INFO_USE_DESKTOP_NAME, #OL_APP_INFO_USE_DESKTOP_ICON, or
 * #OL_APP_INFO_USE_DESKTOP_CMDLINE is used, #OlAppInfo will try to find the
 * desktop file by the command name in #cmdline.
 * 
 * @param cmdline The command line, cannot be #NULL.
 * @param name 
 * @param icon_name 
 * @param flags 
 * 
 * @return If failed, return #NULL, and the #error will be set. Otherwise
 * return the GAppInfo instance.
 */
OlAppInfo *ol_app_info_new (const char *cmdline,
                            const char *name,
                            const char *icon_name,
                            enum OlAppInfoFlags flags,
                            GError **error);

G_END_DECLS

#endif /* _OL_APP_INFO_H_ */
#include <glib.h>

