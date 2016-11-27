/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldi@gmail.com>
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
 * along with OSD Lyrics.  If not, see <http://www.gnu.org/licenses/>. 
 */
#include <libnotify/notify.h>
#include "config.h"
#include "ol_notify.h"
#include "ol_intl.h"
#include "ol_utils.h"
#include "ol_debug.h"

static const char *UNKNOWN_TITLE = N_("Unknown title");
static const char *UNKNOWN_ARTIST = N_("Unknown artist");
static const char *INFO_FORMAT = "%s";
static const char *INFO_FORMAT_ALBUM = "%s\n<i>%s</i>";
static const int DEFAULT_TIMEOUT = -1;

static NotifyNotification *notify = NULL;

static gboolean _init ();
static NotifyNotification *_get_notify (const char *summary,
                                        const char *body,
                                        const char *icon);

static NotifyNotification *
_get_notify (const char *summary,
             const char *body,
             const char *icon)
{
  ol_debugf ("summary: %s\n"
             "body: %s\n"
             "icon: %s\n",
             summary,
             body,
             icon);
  if (notify == NULL)
  {
#ifdef HAVE_LIBNOTIFY_0_7
    notify = notify_notification_new (summary,
                                      body,
                                      icon);
#else
    notify = notify_notification_new (summary,
                                      body,
                                      icon,
                                      NULL);
#endif
  }
  else
  {
    notify_notification_update (notify,
                                summary,
                                body,
                                icon);
  }
  return notify;
}

static gboolean
_init ()
{
  if (!notify_is_initted())
    notify_init (_(PROGRAM_NAME));
  return TRUE;
}

void
ol_notify_init ()
{
  _init ();
}

void
ol_notify_unload ()
{
  if (notify != NULL)
    g_object_unref (notify);
  notify_uninit ();
}

void
ol_notify_music_change (OlMetadata *info, const char *icon)
{
  ol_assert (info != NULL);
  if (!_init ())
  {
    return;
  }
  char *art = NULL;
  const char *title = ol_metadata_get_title (info);
  const char *artist = ol_metadata_get_artist (info);
  if (title == NULL && artist == NULL)
    return;
  if (title == NULL || !title[0])
    title = _(UNKNOWN_TITLE);
  if (artist == NULL)
    artist = _(UNKNOWN_ARTIST);
  const char *album = ol_metadata_get_album (info);
  char *body = NULL;
  if (album == NULL)
  {
    body = g_markup_printf_escaped (INFO_FORMAT,
                                    artist);
  }
  else
  {
    body = g_markup_printf_escaped (INFO_FORMAT_ALBUM,
                                    artist,
                                    album);
  }
  if (ol_metadata_get_art (info) != NULL)
  {
    const char *art_uri = ol_metadata_get_art (info);
    if (art_uri[0] == '/')
      art = g_strdup (art_uri);
    else if (g_str_has_prefix (art_uri, "file://"))
      art = g_filename_from_uri (art_uri, NULL, NULL);
    if (art && !ol_path_is_file (art))
    {
      g_free (art);
      art = NULL;
    }
  }
  NotifyNotification *music_notify = _get_notify (title, body, art ? art : icon);
  notify_notification_set_timeout (music_notify,
                                   DEFAULT_TIMEOUT);
  notify_notification_show (music_notify, NULL);
  g_free (body);
  g_free (art);
}
