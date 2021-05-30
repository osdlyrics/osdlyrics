/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldier@gmail.com>
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
#ifndef _OL_LRC_H_
#define _OL_LRC_H_

#include <glib-object.h>

#define OL_TYPE_LRC                          \
  (ol_lrc_get_type ())
#define OL_LRC(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, OL_TYPE_LRC, OlLrc))
#define OL_LRC_CLASS(klass)                                        \
  (G_TYPE_CHECK_CLASS_CAST (klass, OL_TYPE_LRC, OlLrcClass))
#define OL_IS_LRC(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj, OL_TYPE_LRC))
#define OL_IS_LRC_CLASS(klass)                       \
  (G_TYPE_CHECK_CLASS_TYPE (klass, OL_TYPE_LRC))
#define OL_LRC_GET_CLASS(obj)                                        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OL_TYPE_LRC, OlLrcClass))


typedef struct _OlLrc OlLrc;
struct _OlLrc
{
  GObject parent;
  gpointer priv; /** Private data pointer */
};

typedef struct _OlLrcClass OlLrcClass;
struct _OlLrcClass
{
  GObjectClass parent_class;
};

#include "ol_lyrics.h"

GType ol_lrc_get_type (void);

/**
 * @brief An iterator to iterate the content of the LRC file
 *
 * The iterator points to a line of the content. From the iterator you can get the
 * ID, timestamp, and lyric text of the line. It is guanranteed that you can visit
 * all the content in the lyrics in ascending order of timestamp by iterating it.
 */

typedef struct _OlLrcIter OlLrcIter;
struct _OlLrcIter;

/**
 * @brief Create a new OlLrc instance
 *
 * @return The OlLrc instance of the file, or NULL if filename doesn't exist.
 *         You should free it with g_object_unref.
 */
OlLrc *ol_lrc_new (OlLyrics *lyric_proxy,
                   const gchar *uri);

/**
 * Sets the LRC attributes from GVariant
 *
 * This function is intented for D-Bus communication. The returned attributes of
 * GetLyrics or GetCurrentLyrics call should be able to pass as the attribute
 * parameter directly.
 *
 * The GVariant contains a dictionary with strings as keys and strings as values.
 *
 * @param lrc
 * @param attribute GVariant of type a{ss}
 */
void ol_lrc_set_attributes_from_variant (OlLrc *lrc,
                                         GVariant *attribute);

/**
 * Sets the LRC content from GVariant
 *
 * This function is intented for D-Bus communication. The returned content of
 * GetLyrics or GetCurrentLyrics call should be able to pass as the content
 * parameter directly.
 *
 * The GVariant contains an array of a dictionary with strings as keys and
 * variants as values.
 *
 * It is guaranteed that there is at least one line in lrc after setting the
 * content
 *
 * @param lrc
 * @param content GVariant of type a{ss}
 */
void ol_lrc_set_content_from_variant (OlLrc *lrc,
                                      GVariant *content);

/**
 * Gets the value of an attribute of an LRC file
 *
 * @param lrc
 * @param key
 *
 * @return NULL if the attribute doesn't exist. Otherwise return the value
 *         of the key.
 */
const char *ol_lrc_get_attribute (OlLrc *lrc,
                                  const char *key);
/**
 * @brief Get the number of lyric items
 *
 * @param lrc
 *
 * @return The number of lyric items
 */
guint ol_lrc_get_item_count (OlLrc *lrc);

/**
 * Update the offset of the lrc file
 *
 * If the #lyrics_proxy is set in ol_lrc_new(), the ol_lyrics_set_offset() will be
 * invoked automatically. To avoid performance issue, the ol_lyrics_set_offset() will
 * not be called right after ol_lrc_set_offset() is called. The #OlLrc object will
 * sync the offset change 1 second after last ol_lrc_set_offset() call.
 *
 * @param lrc
 * @param offset The offset of which should be ajusted
 */
void ol_lrc_set_offset (OlLrc *lrc,
                        int offset);

/**
 * Get the offset of the LRC file
 *
 * @param lrc
 *
 * @return the current offset time
 */
int ol_lrc_get_offset (OlLrc *lrc);

/**
 * Sets the duration of the track.
 *
 * The duration is used to calculate the duration and percentage of the last
 * line of lyrics. If the duration is less than the start time of the last line,
 * it is invalid and will not be used.
 *
 * @param lrc
 * @param duration
 */
void ol_lrc_set_duration (OlLrc *lrc, guint64 duration);

/**
 * Gets the duration of the track.
 *
 * @param lrc
 *
 * @return The duration of the track.
 */
guint64 ol_lrc_get_duration (OlLrc *lrc);
/**
 * @brief Get the URI of the LRC file
 *
 * @param lrc
 *
 * @return The URI or NULL. The lrc owns the the uri, so don't modify or free it.
 */
const char *ol_lrc_get_uri (OlLrc *lrc);


/**
 * Frees an iterator.
 *
 * @param iter
 */
void ol_lrc_iter_free (OlLrcIter *iter);

/**
 * @brief Gets an iterator by the ID of the line
 *
 * @param lrc
 * @param id
 *
 * @return The lyric iterator with the id, or NULL if not found. Should be freed
 *         with ol_lrc_iter_free
 */
OlLrcIter *ol_lrc_iter_from_id (OlLrc *lrc, guint id);

/**
 * @brief Gets the lyric iterator that fits the given timestamp
 *
 * The returned lyric's start time should be less or equal than the given time,
 * while it's end time should be larger than the given time.
 *
 * If the given timestamp is less than the first line, the first line will be
 * returned. If the given timestamp is greater than the duration of the track,
 * the last line will be returned.
 *
 * This function returns the upperbound, which means that if there are more than
 * one line starts with the given timestamp, this function return returns the last
 * on.
 *
 * @param lrc The LRC file
 * @param timestamp The timestamp
 *
 * @return The iterator of the timestamp. Should be freed with ol_lrc_iter_free
 */
OlLrcIter *ol_lrc_iter_from_timestamp (OlLrc *lrc,
                                       gint64 timestamp);
/**
 * @brief Move to the previous line of lyrics
 *
 * @param iter
 *
 * @return FALSE if the given iter is the first one. TRUE if succeed.
 */
gboolean ol_lrc_iter_prev (OlLrcIter *iter);

/**
 * @brief Move to the next line of lyrics
 *
 * @param iter The iterator
 *
 * @return FALSE if the given iter is the last one. TRUE if succeed.
 */
gboolean ol_lrc_iter_next (OlLrcIter *iter);

/**
 * Move the iterator to the line of the given ID
 *
 * @param iter
 * @param id
 *
 * @return TRUE if success, or FALSE if the id is out of range.
 */
gboolean ol_lrc_iter_move_to (OlLrcIter *iter, guint id);

/**
 * Get the information of the line represented by the iterator and move the
 * iterator to the next.
 *
 * This function is convenient to iterate the lyrics using a while loop:
 *
 * while (ol_lrc_iter_loop (iter, &id, &timestamp, &text))
 * {
 *   do_someting ();
 * }
 *
 * @param iter
 * @param id The return location of id of the current iterator. NULL is OK.
 * @param timestamp The return location of id of the current timestamp. NULL is OK.
 * @param text The return location of id of the current lyric text. NULL is OK.
 *             Should NOT be modified or freed.
 *
 * @return FALSE if the iterator reaches the end of the lyrics. Otherwise TRUE will
 *         be returned and the id, timestamp and text will be set.
 */
gboolean ol_lrc_iter_loop (OlLrcIter *iter,
                           guint *id,
                           gint64 *timestamp,
                           const char **text);

/**
 * Get the ID of the line.
 *
 * The ID is the index of the lyric line.
 * @param iter
 *
 * @return
 */
guint ol_lrc_iter_get_id (OlLrcIter *iter);

/**
 * @brief Get the timestamp of the line, in milliseconds
 *
 * The time is calculated with offset of LRC
 *
 * @param iter The iterator
 *
 * @return The timestamp of the line represented by the iterator.
 */
gint64 ol_lrc_iter_get_timestamp (OlLrcIter *iter);

/**
 * @brief Get the text of the line
 *
 * @param item
 *
 * @return The text of the lyric item.
 */
const char *ol_lrc_iter_get_text(OlLrcIter *iter);

/**
 * @brief Get the id of the line
 *
 * The id differs with different lyric item, and is in ascending order by
 * timestamp. Ids start from 0.
 *
 * @param item
 *
 * @return The id of the item, or 0 if the item is invalid.
 */
guint ol_lrc_iter_get_id (OlLrcIter *iter);

/**
 * Gets the duration of the line
 *
 * The duration of the line is the difference between the timestamp of this and the
 * next line.
 *
 * If the line is the last one, the duration is the duration of the track minus the
 * timestamp of the line. If the duration is less than the timestamp of the last
 * line, the duration of the last line will be 5 seconds.
 *
 * @param iter
 *
 * @return The duration of the line. Note that this may be 0.
 */
guint64 ol_lrc_iter_get_duration (OlLrcIter *iter);

/**
 * Figure out how much of an single lyric text has been played when the position
 * of the track reaches given value.
 *
 * The returned value is guaranteed in the range of [0.0, 1.0].
 *
 * @param iter
 * @param time_ms The position of the track, in milliseconds.
 *
 * @return
 */
gdouble ol_lrc_iter_compute_percentage (OlLrcIter *iter,
                                        gint64 time_ms);

/**
 * Determine if a iterator is out of range.
 *
 * @param iter
 *
 * @return
 */
gboolean ol_lrc_iter_is_valid (OlLrcIter *iter);

#endif /* _OL_LRC_H_ */
