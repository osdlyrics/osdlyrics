/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier
 *
 * This file is part of OSD Lyrics.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "ol_lrc.h"
#include "ol_debug.h"

static const int DEFAULT_LAST_DURATION = 5000;

struct _OlLrcIter
{
  guint id;
  OlLrc *lrc;
};

struct OlLrcItem
{
  int timestamp;
  char *text;
};

typedef struct _OlLrcPrivate OlLrcPrivate;

struct _OlLrcPrivate
{
  char *uri;
  int offset;
  GHashTable *metadata;
  GPtrArray *items;
  guint64 duration;
  OlLyrics *lyric_proxy;
  guint save_offset_timer;
};

struct SaveOffsetData
{
  OlLrc *lrc;
  int offset;
};

#define OL_LRC_GET_PRIVATE(object) \
    ((OlLrcPrivate *)((OL_LRC(object))->priv))

/* -------------- OlLrc private methods ------------------*/
static void ol_lrc_finalize (GObject *object);
/* -------------- OlLrcItem private methods --------------*/
static struct OlLrcItem *ol_lrc_item_new (gint64 timestamp, const gchar *text);
static void ol_lrc_item_free (struct OlLrcItem *item);
/* -------------- OlLrcIter private methods --------------*/
static OlLrcIter *ol_lrc_iter_new (OlLrc *lrc, guint index);
static struct OlLrcItem *ol_lrc_iter_get_item (OlLrcIter *iter);
/* ---------------Save offset functions ----------------- */
static gboolean _save_offset_timeout (OlLrc *lrc);

G_DEFINE_TYPE_WITH_PRIVATE (OlLrc, ol_lrc, G_TYPE_OBJECT);

static void
ol_lrc_class_init (OlLrcClass *klass)
{
  GObjectClass *gklass = G_OBJECT_CLASS (klass);
  gklass->finalize = ol_lrc_finalize;
}

static void
ol_lrc_init (OlLrc *lrc)
{
  /* Allocate Private data structure */
  (OL_LRC(lrc))->priv = \
      (OlLrcPrivate *) g_malloc0(sizeof(OlLrcPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_LRC(lrc))->priv != NULL)
  {
    OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
    priv->items = g_ptr_array_new_with_free_func ((GDestroyNotify) ol_lrc_item_free);
    /* ensure there is at lease one line */
    g_ptr_array_add (priv->items, ol_lrc_item_new (0, ""));

    priv->metadata = g_hash_table_new_full (g_str_hash,
                                            g_str_equal,
                                            (GDestroyNotify) g_free,
                                            (GDestroyNotify) g_free);
    priv->save_offset_timer = 0;
    priv->offset = 0;
  }
};

static void
ol_lrc_dispose(GObject *object)
{
  OlLrc *self = (OlLrc *)object;
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (self);
  /* Check if not NULL! To avoid calling dispose multiple times */
  if(priv != NULL)
  {
    /* Deallocate contents of the private data, if any */
    /* Deallocate private data structure */
    g_free(priv);
    /* And finally set the opaque pointer back to NULL, so that
     *  we don't deallocate it twice. */
    (OL_LRC(self))->priv = NULL;
  }
}

OlLrc *
ol_lrc_new (OlLyrics *lyric_proxy,
            const gchar *uri)
{
  OlLrc *lrc = OL_LRC (g_object_new (OL_TYPE_LRC, NULL));
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  priv->uri = g_strdup (uri);
  if (lyric_proxy)
    priv->lyric_proxy = g_object_ref (lyric_proxy);
  else
    priv->lyric_proxy = NULL;
  return lrc;
}

static void
ol_lrc_finalize (GObject *object)
{
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (object);
  if (priv->save_offset_timer > 0)
  {
    g_source_remove (priv->save_offset_timer);
    priv->save_offset_timer = 0;
    ol_lyrics_set_offset (priv->lyric_proxy,
                          priv->uri,
                          priv->offset);
  }
  g_ptr_array_free (priv->items, TRUE);
  g_hash_table_destroy (priv->metadata);
  g_free (priv->uri);
  if (priv->lyric_proxy)
  {
    g_object_unref (priv->lyric_proxy);
    priv->lyric_proxy = NULL;
  }
  priv->items = NULL;
  priv->metadata = NULL;
  priv->uri = NULL;
  G_OBJECT_CLASS (ol_lrc_parent_class)->finalize (object);
}

void
ol_lrc_set_attributes_from_variant (OlLrc *lrc,
                                    GVariant *attributes)
{
  ol_assert (OL_IS_LRC (lrc));
  ol_assert (attributes != NULL);
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  g_hash_table_remove_all (priv->metadata);
  GVariantIter *iter = NULL;
  g_variant_get (attributes, "a{ss}", &iter);
  gchar *key, *value;
  while (g_variant_iter_loop (iter, "{ss}", &key, &value))
  {
    g_hash_table_insert (priv->metadata, g_strdup (key), g_strdup (value));
    ol_debugf ("LRC attribute: %s -> %s\n", key, value);
  }
  g_variant_iter_free (iter);
  const char *offset = NULL;
  if ((offset = g_hash_table_lookup (priv->metadata, "offset")) != NULL)
    priv->offset = atoi (offset);
  else
    priv->offset = 0;
}

void
ol_lrc_set_content_from_variant (OlLrc *lrc,
                                 GVariant *content)
{
  ol_assert (OL_IS_LRC (lrc));
  ol_assert (content != NULL);
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  g_ptr_array_remove_range (priv->items, 0, priv->items->len);
  GVariantIter *iter = NULL;
  g_variant_get (content, "aa{sv}", &iter);
  GVariantIter *dict_iter = NULL;
  while (g_variant_iter_loop (iter, "a{sv}", &dict_iter))
  {
    gchar *key = NULL;
    GVariant *value = NULL;
    if (g_variant_iter_n_children (dict_iter) < 3)
    {
      ol_errorf ("The attributes of a lyric line is not enough, expect 3 attributes "
                 "(id, timestame, text) but there are only %d attributes.\n",
                 g_variant_iter_n_children (dict_iter));
      continue;
    }
    guint id = 0;
    gint64 timestamp = 0;
    const gchar *text = NULL;
    gboolean has_id = FALSE, has_timestamp = FALSE;
    while (g_variant_iter_loop (dict_iter, "{sv}", &key, &value))
    {
      if (strcmp (key, "id") == 0)
      {
        id = g_variant_get_uint32 (value);
        has_id = TRUE;
      }
      else if (strcmp (key, "timestamp") == 0)
      {
        timestamp = g_variant_get_int64 (value);
        has_timestamp = TRUE;
      }
      else if (strcmp (key, "text") == 0)
      {
        text = g_variant_get_string (value, NULL);
      }
      else
      {
        ol_errorf ("Unknown line attribute: %s\n", key);
      }
    } /* for dict_iter */
    if (has_id && has_timestamp && text != NULL)
    {
      g_ptr_array_add (priv->items, ol_lrc_item_new (timestamp, text));
      ol_infof ("new lyric item: %d, %d: %s\n", (int)id, (int)timestamp, text);
    }
    else
    {
      if (!has_id)
        ol_errorf ("missing id in lyric line\n");
      if (!has_timestamp)
        ol_errorf ("missing timestamp in lyric line\n");
      if (!text)
        ol_errorf ("missing text in lyric line\n");
    } /* if */
  } /* for iter */
  g_variant_iter_free (iter);
  /* Ensure there are at least one item */
  if (priv->items->len == 0)
    g_ptr_array_add (priv->items, ol_lrc_item_new (0, ""));
}

const char *
ol_lrc_get_attribute (OlLrc *lrc,
                      const char *key)
{
  ol_assert_ret (OL_IS_LRC (lrc), NULL);
  ol_assert_ret (key != NULL, NULL);
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  return g_hash_table_lookup (priv->metadata, key);
}

guint
ol_lrc_get_item_count (OlLrc *lrc)
{
  ol_assert_ret (OL_IS_LRC (lrc), 0);
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  return priv->items->len;
}

static gboolean
_save_offset_timeout (OlLrc *lrc)
{
  ol_assert_ret (OL_IS_LRC (lrc), FALSE);
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  ol_lyrics_set_offset (priv->lyric_proxy,
                        priv->uri,
                        priv->offset);
  priv->save_offset_timer = 0;
  return FALSE;
}

void
ol_lrc_set_offset (OlLrc *lrc,
                   int offset)
{
  ol_assert (OL_IS_LRC (lrc));
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  priv->offset = offset;
  g_hash_table_replace (priv->metadata,
                        g_strdup ("offset"),
                        g_strdup_printf ("%d", priv->offset));
  if (priv->lyric_proxy)
  {
    if (priv->save_offset_timer > 0)
    {
      g_source_remove (priv->save_offset_timer);
      priv->save_offset_timer = 0;
    }
    priv->save_offset_timer = g_timeout_add (1000,
                                             (GSourceFunc) _save_offset_timeout,
                                             lrc);
  }
}

int
ol_lrc_get_offset (OlLrc *lrc)
{
  ol_assert_ret (OL_IS_LRC (lrc), 0);
  return OL_LRC_GET_PRIVATE (lrc)->offset;
}

OlLrcIter *
ol_lrc_iter_from_id (OlLrc *lrc, guint id)
{
  ol_assert_ret (OL_IS_LRC (lrc), NULL);
  if (id < ol_lrc_get_item_count (lrc))
    return ol_lrc_iter_new (lrc, id);
  else
    return NULL;
}

OlLrcIter *
ol_lrc_iter_from_timestamp (OlLrc *lrc,
                            gint64 timestamp)
{
  ol_assert_ret (OL_IS_LRC (lrc), NULL);
  int low = 0;
  int high = ol_lrc_get_item_count (lrc) - 1;
  OlLrcIter *iter = ol_lrc_iter_from_id (lrc, 0);
  /* Binary search */
  while (low < high)
  {
    int mid = (low + high + 1) / 2;
    ol_lrc_iter_move_to (iter, mid);
    if (ol_lrc_iter_get_timestamp (iter) <= timestamp)
      low = mid;
    else
      high = mid - 1;
  }
  if (low == high)                    /* found */
  {
    ol_lrc_iter_move_to (iter, low);
  }
  else
  {
    ol_errorf ("low(%d) != high(%d), this should not happen\n", low, high);
  }
  return iter;
}

const char *
ol_lrc_get_uri (OlLrc *lrc)
{
  ol_assert_ret (OL_IS_LRC (lrc), NULL);
  return OL_LRC_GET_PRIVATE (lrc)->uri;
}

void
ol_lrc_set_duration (OlLrc *lrc, guint64 duration)
{
  ol_assert (OL_IS_LRC (lrc));
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  priv->duration = duration;
}

guint64
ol_lrc_get_duration (OlLrc *lrc)
{
  ol_assert_ret (OL_IS_LRC (lrc), 0);
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (lrc);
  return priv->duration;
}

static struct OlLrcItem *
ol_lrc_item_new (gint64 timestamp, const gchar *text)
{
  if (text == NULL)
    text = "";
  struct OlLrcItem *item = g_new (struct OlLrcItem, 1);
  item->timestamp = timestamp;
  item->text = g_strdup (text);
  return item;
}

static void
ol_lrc_item_free (struct OlLrcItem *item)
{
  if (item == NULL)
    return;
  g_free (item->text);
  g_free (item);
}

static OlLrcIter *
ol_lrc_iter_new (OlLrc *lrc, guint index)
{
  ol_assert_ret (OL_IS_LRC (lrc), NULL);
  ol_assert_ret (index < ol_lrc_get_item_count (lrc), NULL);
  OlLrcIter *iter = g_new (OlLrcIter, 1);
  iter->lrc = g_object_ref (lrc);
  iter->id = index;
  return iter;
}

void
ol_lrc_iter_free (OlLrcIter *iter)
{
  if (iter == NULL)
    return;
  g_object_unref (iter->lrc);
  g_free (iter);
}

gboolean
ol_lrc_iter_prev (OlLrcIter *iter)
{
  ol_assert_ret (iter != NULL, FALSE);
  if (iter->id == 0)
    return FALSE;
  iter->id--;
  return TRUE;
}

gboolean
ol_lrc_iter_next (OlLrcIter *iter)
{
  ol_assert_ret (iter != NULL, FALSE);
  iter->id++;
  if (iter->id >= ol_lrc_get_item_count (iter->lrc))
    return FALSE;
  return TRUE;
}

gboolean
ol_lrc_iter_move_to (OlLrcIter *iter, guint id)
{
  ol_assert_ret (iter != NULL, FALSE);
  if (id >= ol_lrc_get_item_count (iter->lrc))
    return FALSE;
  iter->id = id;
  return TRUE;
}

gboolean
ol_lrc_iter_loop (OlLrcIter *iter,
                  guint *id,
                  gint64 *timestamp,
                  const char **text)
{
  ol_assert_ret (iter != NULL, FALSE);
  if (!ol_lrc_iter_is_valid (iter))
    return FALSE;
  if (id)
    *id = ol_lrc_iter_get_id (iter);
  if (timestamp)
    *timestamp = ol_lrc_iter_get_timestamp (iter);
  if (text)
    *text = ol_lrc_iter_get_text (iter);
  ol_lrc_iter_next (iter);
  return TRUE;
}

guint
ol_lrc_iter_get_id (OlLrcIter *iter)
{
  ol_assert_ret (iter != NULL, 0);
  return iter->id;
}

static struct OlLrcItem *
ol_lrc_iter_get_item (OlLrcIter *iter)
{
  ol_assert_ret (iter != NULL, NULL);
  if (iter->id >= ol_lrc_get_item_count (iter->lrc))
  {
    ol_errorf ("LRC Iter is out of range. Don't use the iter after resetting the content or reaching the end.\n");
    return NULL;
  }
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (iter->lrc);
  struct OlLrcItem *item = g_ptr_array_index (priv->items, iter->id);
  return item;
}

gint64
ol_lrc_iter_get_timestamp (OlLrcIter *iter)
{
  struct OlLrcItem *item = ol_lrc_iter_get_item (iter);
  if (!item)
    return 0;
  return item->timestamp - ol_lrc_get_offset (iter->lrc);
}

const char *
ol_lrc_iter_get_text(OlLrcIter *iter)
{
  struct OlLrcItem *item = ol_lrc_iter_get_item (iter);
  if (!item)
    return NULL;
  return item->text;
}

gboolean
ol_lrc_iter_is_valid (OlLrcIter *iter)
{
  ol_assert_ret (iter != NULL, FALSE);
  return iter->id < ol_lrc_get_item_count (iter->lrc);
}

guint64
ol_lrc_iter_get_duration (OlLrcIter *iter)
{
  struct OlLrcItem *curr = ol_lrc_iter_get_item (iter);
  if (!curr)
    return 0;
  OlLrcPrivate *priv = OL_LRC_GET_PRIVATE (iter->lrc);
  if (iter->id < ol_lrc_get_item_count (iter->lrc) - 1)
  {
    /* Not the last one */
    struct OlLrcItem *next = g_ptr_array_index (priv->items, iter->id + 1);
    return next->timestamp - curr->timestamp;
  }
  else
  {
    gint64 duration = ol_lrc_get_duration (iter->lrc);
    gint64 timestamp = ol_lrc_iter_get_timestamp (iter);
    if (duration <= timestamp)
      return DEFAULT_LAST_DURATION;
    else
      return duration - timestamp;
  }
}

gdouble
ol_lrc_iter_compute_percentage (OlLrcIter *iter,
                                gint64 time_ms)
{
  ol_assert_ret (ol_lrc_iter_is_valid (iter), 0.0);
  gint64 timestamp = ol_lrc_iter_get_timestamp (iter);
  if (time_ms <= timestamp)
    return 0.0;
  /* use int64 instead of uint64 to avoid negative sum problem */
  gint64 duration = ol_lrc_iter_get_duration (iter);
  if (time_ms >= timestamp + duration)
    return 1.0;
  return (gdouble) (time_ms - timestamp) / (gdouble) duration;
}
