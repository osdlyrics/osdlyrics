/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011  Tiger Soldier <tigersoldier@gmail.com>
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
#ifndef __OL_LYRICS_H__
#define __OL_LYRICS_H__

#include <gio/gio.h>
#include "ol_metadata.h"

G_BEGIN_DECLS

#define OL_TYPE_LYRICS                          \
  (ol_lyrics_get_type ())
#define OL_LYRICS(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, OL_TYPE_LYRICS, OlLyrics))
#define OL_LYRICS_CLASS(klass)                                        \
  (G_TYPE_CHECK_CLASS_CAST (klass, OL_TYPE_LYRICS, OlLyricsClass))
#define OL_IS_LYRICS(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj, OL_TYPE_LYRICS))
#define OL_IS_LYRICS_CLASS(klass)                       \
  (G_TYPE_CHECK_CLASS_TYPE (klass, OL_TYPE_LYRICS))
#define OL_LYRICS_GET_CLASS(obj)                                        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OL_TYPE_LYRICS, OlLyricsClass))

typedef struct _OlLyrics OlLyrics;
struct _OlLyrics
{
  GDBusProxy parent;
  gpointer priv;
};

typedef struct _OlLyricsClass OlLyricsClass;
struct _OlLyricsClass
{
  GDBusProxyClass parent_class;
};

#include "ol_lrc.h"

GType ol_lyrics_get_type (void) G_GNUC_CONST;

OlLyrics *ol_lyrics_new (GError **error);

void ol_lyrics_proxy_new_async (GCancellable *cancellable,
                                GAsyncReadyCallback callback,
                                gpointer user_data,
                                GError **error);

OlLyrics *ol_lyrics_proxy_new_finish (GAsyncResult *res,
                                      GError **error);

/* D-Bus method calls: */

/**
 * Gets the lyrics of current playing track
 *
 * @param proxy
 *
 * @return NULL if no lyrics found. Otherwise return an #OlLrc object
 *         representing the lyrics. You should use g_object_unref() to free
 *         the returned #OlLrc object.
 */
OlLrc *ol_lyrics_get_current_lyrics (OlLyrics *proxy);

/**
 * Gets the lyrics assigned to the given metadata
 *
 * @param proxy
 * @param metadata The metadata, must not be #NULL
 *
 * @return NULL if no lyrics found. Otherwise return an #OlLrc object
 *         representing the lyrics. You should use g_object_unref() to free
 *         the returned #OlLrc object.
 */
OlLrc *ol_lyrics_get_lyrics (OlLyrics *proxy,
                             OlMetadata *metadata);

/**
 * Gets the content of the LRC file assigned to the specified metadata in
 * plain text.
 *
 * @param proxy
 * @param metadata The metadata, must not be #NULL.
 * @param uri The return location of the uri of the LRC file. Should be freed with
 *        g_free(). Can be #NULL.
 * @param content The return location of the content of the LRC file. Should be
 *        freed with g_free(). Can be #NULL.
 *
 * @return #TRUE if there are LRC file assigned to #metadata. If #metadata is
 *         assigned to not show any lyrics, #TRUE will be returned and #content will
 *         pointed to an empty string rather than #NULL.
 */
gboolean ol_lyrics_get_raw_lyrics (OlLyrics *proxy,
                                   OlMetadata *metadata,
                                   char **uri,
                                   char **content);

/**
 * Gets the content of the LRC file assigned to the current track in
 * plain text.
 *
 * @param proxy
 * @param uri The return location of the uri of the LRC file. Should be freed with
 *        g_free(). Can be #NULL.
 * @param content The return location of the content of the LRC file. Should be
 *        freed with g_free(). Can be #NULL.
 *
 * @return #TRUE if there are LRC file assigned to #metadata. If #metadata is
 *         assigned to not show any lyrics, #TRUE will be returned and #content will
 *         pointed to an empty string rather than #NULL.
 */
gboolean ol_lyrics_get_current_raw_lyrics (OlLyrics *proxy,
                                           char **uri,
                                           char **content);

/**
 * Sets the content of LRC file for given metadata.
 *
 * @param proxy
 * @param metadata The metadata, must not be #NULL.
 * @param content The content of the LRC file, must not be #NULL
 * @param error
 *
 * @return If succeed, the URI of the new LRC file of the content will be returned.
 *         If failed, #NULL will be returned and the error will be set. The returned
 *         URI should be freed with g_free().
 */
gchar *ol_lyrics_set_content (OlLyrics *proxy,
                              OlMetadata *metadata,
                              const char *content,
                              GError **error);

/**
 * Assigns an URI to given metadata.
 *
 * The URI should be valid URI format defined in the D-Bus specification of the
 * daemon
 * @param proxy
 * @param metadata The metadata to be assigned. Must not be #NULL.
 * @param uri The URI of the LRC file. Must not be #NULL.
 * @param error
 *
 * @return If success, return #TRUE. Otherwise return #FALSE and #error will be
 *         set.
 */
gboolean ol_lyrics_assign (OlLyrics *proxy,
                           OlMetadata *metadata,
                           const char *uri,
                           GError **error);

/**
 * Sets the offset of a given LRC file.
 *
 * @param proxy
 * @param uri The URI of the LRC file. Must not be #NULL.
 * @param offset The offset.
 *
 * @return #TRUE if success. Otherwise return #FALSE and #error will be
 *         set.
 */
gboolean ol_lyrics_set_offset (OlLyrics *proxy,
                               const char *uri,
                               gint offset);
G_END_DECLS

#endif /* __OL_LYRICS_H__ */
