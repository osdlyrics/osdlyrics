/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2012  Tiger Soldier <tigersoldier@gmail.com>
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

#include <string.h>
#include <gio/gio.h>
#include "ol_lyric_source.h"
#include "ol_consts.h"
#include "ol_marshal.h"
#include "ol_debug.h"

G_DEFINE_TYPE (OlLyricSource, ol_lyric_source, G_TYPE_DBUS_PROXY);
G_DEFINE_TYPE (OlLyricSourceTask, ol_lyric_source_task, G_TYPE_OBJECT);
G_DEFINE_TYPE (OlLyricSourceSearchTask,
              ol_lyric_source_search_task,
              OL_TYPE_LYRIC_SOURCE_TASK);
G_DEFINE_TYPE (OlLyricSourceDownloadTask,
              ol_lyric_source_download_task,
              OL_TYPE_LYRIC_SOURCE_TASK);
G_DEFINE_TYPE (OlLyricSourceCandidate,
              ol_lyric_source_candidate,
              G_TYPE_OBJECT);

#define OL_LYRIC_SOURCE_GET_PRIVATE(obj) \
  ((OlLyricSourcePrivate *)((OL_LYRIC_SOURCE(obj))->priv))

#define OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE(obj) \
  ((OlLyricSourceCandidatePrivate *)((OL_LYRIC_SOURCE_CANDIDATE(obj))->priv))

#define OL_LYRIC_SOURCE_TASK_GET_PRIVATE(obj) \
  ((OlLyricSourceTaskPrivate *)((OL_LYRIC_SOURCE_TASK(obj))->priv))

struct _OlLyricSourceInfo
{
  gchar *id;
  gchar *name;
  gboolean enabled;
};

typedef struct _OlLyricSourcePrivate OlLyricSourcePrivate;
struct _OlLyricSourcePrivate
{
  GHashTable *search_tasks;
  GHashTable *download_tasks;
};

typedef struct _OlLyricSourceCandidatePrivate OlLyricSourceCandidatePrivate;
struct _OlLyricSourceCandidatePrivate
{
  gchar *title;
  gchar *artist;
  gchar *album;
  gchar *comment;
  gchar *sourceid;
  GVariant *downloadinfo;
};

typedef struct _OlLyricSourceTaskPrivate OlLyricSourceTaskPrivate;
struct _OlLyricSourceTaskPrivate
{
  OlLyricSource *source;
  gint taskid;
};

enum {
  SEARCH_SIGNAL_COMPLETE = 0,
  SEARCH_SIGNAL_STARTED,
  SEARCH_SIGNAL_LAST,
};

enum {
  DOWNLOAD_SIGNAL_COMPLETE = 0,
  DOWNLOAD_SIGNAL_LAST,
};

enum {
  CANDIDATE_PROP_NONE = 0,
  CANDIDATE_PROP_TITLE,
  CANDIDATE_PROP_ARTIST,
  CANDIDATE_PROP_ALBUM,
  CANDIDATE_PROP_COMMENT,
  CANDIDATE_PROP_DOWNLOADINFO,
  CANDIDATE_PROP_SOURCEID,
};

enum {
  TASK_PROP_NONE = 0,
  TASK_PROP_SOURCE,
  TASK_PROP_TASKID,
};

static guint _search_signals[SEARCH_SIGNAL_LAST];
static guint _download_signals[DOWNLOAD_SIGNAL_LAST];

/* OlLyricSource */
static void ol_lyric_source_finalize (GObject *object);
static void ol_lyric_source_g_signal (GDBusProxy *proxy,
                                      const gchar *sender_name,
                                      const gchar *signal_name,
                                      GVariant *parameters);
static OlLyricSourceTask *ol_lyric_source_get_search_task (OlLyricSource *source,
                                                           gint taskid);
static OlLyricSourceTask *ol_lyric_source_get_download_task (OlLyricSource *source,
                                                             gint taskid);
static void ol_lyric_source_remove_search_task (OlLyricSource *source,
                                                gint taskid);
static void ol_lyric_source_remove_download_task (OlLyricSource *source,
                                                gint taskid);
static void ol_lyric_source_search_complete_cb (OlLyricSource *source,
                                                GVariant *parameters);
static void ol_lyric_source_search_started_cb (OlLyricSource *source,
                                                GVariant *parameters);
static void ol_lyric_source_download_complete_cb (OlLyricSource *source,
                                                  GVariant *parameters);

static OlLyricSourceInfo *ol_lyric_source_info_new (const gchar *id,
                                                    const gchar *name,
                                                    gboolean enabled);

/* OlLyricSourceCandidate */
static OlLyricSourceCandidate *ol_lyric_source_candidate_new (const gchar *title,
                                                              const gchar *artist,
                                                              const gchar *album,
                                                              const gchar *comment,
                                                              const gchar *sourceid,
                                                              GVariant *downloadinfo);
static OlLyricSourceCandidate *ol_lyric_source_candidate_new_with_variant (GVariant *dict);
static void ol_lyric_source_candidate_finalize (GObject *object);
static void ol_lyric_source_candidate_get_property (GObject *object,
                                                    guint property_id,
                                                    GValue *value,
                                                    GParamSpec *pspec);
static void ol_lyric_source_candidate_set_property (GObject *object,
                                                    guint property_id,
                                                    const GValue *value,
                                                    GParamSpec *pspec);

/* OlLyricSourceTask */
static void ol_lyric_source_task_get_property (GObject *object,
                                               guint property_id,
                                               GValue *value,
                                               GParamSpec *pspec);
static void ol_lyric_source_task_set_property (GObject *object,
                                               guint property_id,
                                               const GValue *value,
                                               GParamSpec *pspec);
static void ol_lyric_source_task_finalize (GObject *object);
static void ol_lyric_source_task_weak_notify (OlLyricSourceTask *task,
                                              OlLyricSource *source);
static void ol_lyric_source_task_call_cancel (OlLyricSourceTask *task,
                                              const gchar *method);

/* OlLyricSourceSearchTask */
static OlLyricSourceSearchTask *ol_lyric_source_search_task_new (OlLyricSource *source,
                                                                 gint taskid);
static void ol_lyric_source_search_task_cancel (OlLyricSourceTask *task);

/* OlLyricSourceDownloadTask */
static OlLyricSourceDownloadTask *ol_lyric_source_download_task_new (OlLyricSource *source,
                                                                     gint taskid);
static void ol_lyric_source_download_task_cancel (OlLyricSourceTask *task);


static void
ol_lyric_source_class_init (OlLyricSourceClass *klass)
{
  GObjectClass *object_class;
  GDBusProxyClass *proxy_class;
  object_class = G_OBJECT_CLASS (klass);
  proxy_class = G_DBUS_PROXY_CLASS (klass);
  object_class->finalize = ol_lyric_source_finalize;
  proxy_class->g_signal = ol_lyric_source_g_signal;
}

static void
ol_lyric_source_init (OlLyricSource *source)
{
  /* Allocate Private data structure */
  (OL_LYRIC_SOURCE(source))->priv = \
  (OlLyricSourcePrivate *) g_malloc0(sizeof(OlLyricSourcePrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_LYRIC_SOURCE(source))->priv != NULL)
  {
    OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
    priv->search_tasks = g_hash_table_new_full (g_direct_hash,
                                                g_direct_equal,
                                                NULL,
                                                (GDestroyNotify)g_object_unref);
    priv->download_tasks = g_hash_table_new_full (g_direct_hash,
                                                  g_direct_equal,
                                                  NULL,
                                                  (GDestroyNotify)g_object_unref);
  }
}

static void
ol_lyric_source_dispose(GObject *object)
{
  OlLyricSource *self = (OlLyricSource *)object;
  OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE(self);
  /* Check if not NULL! To avoid calling dispose multiple times */
  if(priv != NULL)
  {
    /* Deallocate contents of the private data, if any */
    /* Deallocate private data structure */
    g_free(priv);
    /* And finally set the opaque pointer back to NULL, so that
     *  we don't deallocate it twice. */
    (OL_LYRIC_SOURCE(self))->priv = NULL;
  }
}

OlLyricSource *
ol_lyric_source_new (void)
{
  GInitable *ret;
  ret = g_initable_new (OL_TYPE_LYRIC_SOURCE,
                        NULL,   /* cancellable */
                        NULL,
                        "g-flags", G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                        "g-name", OL_SERVICE_DAEMON,
                        "g-bus-type", G_BUS_TYPE_SESSION,
                        "g-object-path", OL_OBJECT_LYRIC_SOURCE,
                        "g-interface-name", OL_IFACE_LYRIC_SOURCE,
                        NULL);
  if (ret != NULL)
    return OL_LYRIC_SOURCE (ret);
  else
    return NULL;
}

static void
ol_lyric_source_finalize (GObject *object)
{
  OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE (object);
  g_hash_table_destroy (priv->search_tasks);
  g_hash_table_destroy (priv->download_tasks);
  priv->search_tasks = NULL;
  priv->download_tasks = NULL;
  G_OBJECT_CLASS (ol_lyric_source_parent_class)->finalize (object);
}

static OlLyricSourceTask *
ol_lyric_source_get_search_task (OlLyricSource *source,
                                 gint taskid)
{
  OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
  return g_hash_table_lookup (priv->search_tasks,
                              GINT_TO_POINTER (taskid));
}

static OlLyricSourceTask *
ol_lyric_source_get_download_task (OlLyricSource *source,
                                   gint taskid)
{
  OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
  return g_hash_table_lookup (priv->download_tasks,
                              GINT_TO_POINTER (taskid));
}

static void
ol_lyric_source_remove_search_task (OlLyricSource *source,
                                    gint taskid)
{
  OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
  g_hash_table_remove (priv->search_tasks, GINT_TO_POINTER (taskid));
}

static void
ol_lyric_source_remove_download_task (OlLyricSource *source,
                                      gint taskid)
{
  OlLyricSourcePrivate *priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
  g_hash_table_remove (priv->download_tasks, GINT_TO_POINTER (taskid));
}

static void
ol_lyric_source_search_complete_cb (OlLyricSource *source,
                                    GVariant *parameters)
{
  GVariantIter *iter = NULL;
  GVariant *dict = NULL;
  GList *result = NULL;
  gint taskid;
  gint statusid;
  OlLyricSourceTask *task;
  g_variant_get (parameters, "(iiaa{sv})", &taskid, &statusid, &iter);
  task = ol_lyric_source_get_search_task (source, taskid);
  if (task == NULL)
  {
    ol_errorf ("Search task %d not exist\n", taskid);
    return;
  }
  if (statusid < 0 || statusid > OL_LYRIC_SOURCE_STATUS_FALIURE)
  {
    ol_errorf ("Invalid search status %d\n", statusid);
    return;
  }
  while (g_variant_iter_loop (iter, "@a{sv}", &dict))
  {
    OlLyricSourceCandidate *candidate;
    candidate = ol_lyric_source_candidate_new_with_variant (dict);
    result = g_list_prepend (result, candidate);
  }
  g_variant_iter_free (iter);
  result = g_list_reverse (result);
  g_signal_emit (task,
                 _search_signals[SEARCH_SIGNAL_COMPLETE],
                 0,
                 statusid,
                 result);
  while (result)
  {
    OlLyricSourceCandidate *candidate = result->data;
    g_object_unref (candidate);
    result = g_list_delete_link (result, result);
  }
  ol_lyric_source_remove_search_task (source, taskid);
}

static void
ol_lyric_source_search_started_cb (OlLyricSource *source,
                                   GVariant *parameters)
{
  OlLyricSourceTask *task;
  int taskid;
  gchar *sourceid;
  gchar *sourcename;
  g_variant_get (parameters, "(i&s&s)", &taskid, &sourceid, &sourcename);
  task = ol_lyric_source_get_search_task (source, taskid);
  if (task == NULL)
  {
    ol_errorf ("Search task %d not exist\n", taskid);
    return;
  }
  g_signal_emit (G_OBJECT (task),
                 _search_signals[SEARCH_SIGNAL_STARTED],
                 0,
                 sourceid,
                 sourcename);
}

static void
ol_lyric_source_download_complete_cb (OlLyricSource *source,
                                      GVariant *parameters)
{
  OlLyricSourceTask *task;
  int taskid;
  int status;
  GVariantIter *iter;
  gchar *content = NULL;
  gsize len = 0;
  g_variant_get (parameters, "(iiay)", &taskid, &status, &iter);
  task = ol_lyric_source_get_download_task (source, taskid);
  if (task == NULL)
  {
    ol_errorf ("Download task %d not exist\n", taskid);
  }
  else if (status < 0 || status > OL_LYRIC_SOURCE_STATUS_FALIURE)
  {
    ol_errorf ("Invalid download status %d\n", status);
  }
  else
  {
    len = g_variant_iter_n_children (iter);
    content = g_new (gchar, len + 1);
    gsize i = 0;
    while (g_variant_iter_loop (iter, "y", &content[i]))
      i++;
    content[len] = '\0';
    g_signal_emit (G_OBJECT (task),
                   _download_signals[DOWNLOAD_SIGNAL_COMPLETE],
                   0,
                   status,
                   content,
                   (guint) len);
    g_free (content);
  }
  g_variant_iter_free (iter);
  ol_lyric_source_remove_download_task (source, taskid);
}

static void
ol_lyric_source_g_signal (GDBusProxy *proxy,
                          const gchar *sender_name,
                          const gchar *signal_name,
                          GVariant *parameters)
{
  OlLyricSource *source = OL_LYRIC_SOURCE (proxy);
  if (strcmp (signal_name, "SearchComplete") == 0)
  {
    ol_lyric_source_search_complete_cb (source, parameters);
  }
  else if (strcmp (signal_name, "SearchStarted") == 0)
  {
    ol_lyric_source_search_started_cb (source, parameters);
  }
  else if (strcmp (signal_name, "DownloadComplete") == 0)
  {
    ol_lyric_source_download_complete_cb (source, parameters);
  }
}

GList *
ol_lyric_source_list_sources (OlLyricSource* source)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE (source), NULL);
  GList *list = NULL;
  GVariant *ret;
  GError *error = NULL;
  ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (source),
                                "ListSources",
                                NULL, /* parameters */
                                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                -1,
                                NULL,
                                &error);
  if (ret)
  {
    GVariantIter *iter;
    g_variant_get (ret, "(aa{sv})", &iter);
    GVariantIter *dictiter;
    while (g_variant_iter_loop (iter, "a{sv}", &dictiter))
    {
      GVariant *value;
      gchar *key;
      gchar *name = NULL, *id = NULL;
      gboolean enabled = FALSE;
      while (g_variant_iter_loop (dictiter, "{sv}", &key, &value))
      {
        if (strcmp (key, "id") == 0 &&
            g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
          id = g_variant_dup_string (value, NULL);
        }
        else if (strcmp (key, "name") == 0 &&
                 g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
        {
          name = g_variant_dup_string (value, NULL);
        }
        else if (strcmp (key, "enabled") == 0 &&
                 g_variant_is_of_type (value, G_VARIANT_TYPE_BOOLEAN))
        {
          enabled = g_variant_get_boolean (value);
        }
      }
      if (id == NULL || name == NULL)
      {
        ol_error ("Missing id or name in lyric source info");
      }
      else
      {
        list = g_list_prepend (list, ol_lyric_source_info_new (id, name, enabled));
      }
      g_free (id);
      g_free (name);
    }
    g_variant_iter_free (iter);
    list = g_list_reverse (list);
    g_variant_unref (ret);
    return list;
  }
  else
  {
    ol_errorf ("Fail to get lyric source list: %s\n", error->message);
    g_error_free (error);
    return NULL;
  }
}

OlLyricSourceSearchTask *
ol_lyric_source_search (OlLyricSource *source,
                        OlMetadata *metadata,
                        GList *source_ids)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE (source), NULL);
  GVariant *ret;
  OlLyricSourceSearchTask *task = NULL;
  GError *error = NULL;
  GList *id;
  GVariantBuilder *idbuilder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  for (id = source_ids; id; id = g_list_next (id))
  {
    g_variant_builder_add (idbuilder, "s", id->data);
  }
  GVariant *mdvalue = ol_metadata_to_variant (metadata);
  ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (source),
                                "Search",
                                g_variant_new ("(@a{sv}as)",
                                               mdvalue,
                                               idbuilder),
                                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                -1,   /* timeout */
                                NULL, /* cancellable */
                                &error);
  g_variant_builder_unref (idbuilder);
  if (ret)
  {
    OlLyricSourcePrivate *priv;
    priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
    int taskid;
    g_variant_get (ret, "(i)", &taskid);
    task = ol_lyric_source_search_task_new (source, taskid);
    g_hash_table_insert (priv->search_tasks,
                         GINT_TO_POINTER (taskid),
                         task);
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Fail to call search: %s\n", error->message);
    g_error_free (error);
  }
  return task;
}

OlLyricSourceSearchTask *
ol_lyric_source_search_default (OlLyricSource *source,
                                OlMetadata *metadata)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE (source), NULL);
  OlLyricSourceSearchTask *task = NULL;
  GList *sources;
  GList *source_ids = NULL;
  sources = ol_lyric_source_list_sources (source);
  for (; sources; sources = g_list_delete_link (sources, sources))
  {
    OlLyricSourceInfo *info = sources->data;
    source_ids = g_list_prepend (source_ids,
                                 g_strdup (ol_lyric_source_info_get_id (info)));
    ol_lyric_source_info_free (info);
  }
  source_ids = g_list_reverse (source_ids);
  task = ol_lyric_source_search (source, metadata, source_ids);
  for (; source_ids; source_ids = g_list_delete_link (source_ids, source_ids))
  {
    g_free (source_ids->data);
  }
  return task;
}

OlLyricSourceDownloadTask *
ol_lyric_source_download (OlLyricSource *source,
                          OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE (source), NULL);
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceDownloadTask *task = NULL;
  GVariant *ret = NULL;
  GError *error = NULL;
  ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (source),
                                "Download",
                                g_variant_new ("(sv)",
                                               ol_lyric_source_candidate_get_sourceid (candidate),
                                               ol_lyric_source_candidate_get_downloadinfo (candidate)),
                                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                -1,   /* timeout */
                                NULL, /* cancellable */
                                &error);
  if (ret)
  {
    OlLyricSourcePrivate *priv;
    priv = OL_LYRIC_SOURCE_GET_PRIVATE (source);
    int taskid = 0;
    g_variant_get (ret, "(i)", &taskid);
    task = ol_lyric_source_download_task_new (source, taskid);
    g_hash_table_insert (priv->download_tasks,
                         GINT_TO_POINTER (taskid),
                         task);
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Fail to call download: %s\n", error->message);
    g_error_free (error);
  }
  return task;
}

/* OlLyricSourceInfo */
static OlLyricSourceInfo *
ol_lyric_source_info_new (const gchar *id,
                          const gchar *name,
                          gboolean enabled)
{
  ol_assert_ret (id != NULL, NULL);
  ol_assert_ret (name != NULL, NULL);
  OlLyricSourceInfo *info = g_new (OlLyricSourceInfo, 1);
  info->id = g_strdup (id);
  info->name = g_strdup (name);
  info->enabled = enabled;
  return info;
}

void
ol_lyric_source_info_free (OlLyricSourceInfo *info)
{
  if (info == NULL) return;
  g_free (info->id);
  g_free (info->name);
  g_free (info);
}

const gchar *
ol_lyric_source_info_get_id (OlLyricSourceInfo *info)
{
  ol_assert_ret (info != NULL, NULL);
  return info->id;
}

const gchar *
ol_lyric_source_info_get_name (OlLyricSourceInfo *info)
{
  ol_assert_ret (info != NULL, NULL);
  return info->name;
}

gboolean
ol_lyric_source_info_get_enabled (OlLyricSourceInfo *info)
{
  ol_assert_ret (info != NULL, FALSE);
  return info->enabled;
}

/* OlLyricSourceCandidate */
static void
ol_lyric_source_candidate_class_init (OlLyricSourceCandidateClass *klass)
{
  GObjectClass *object_class;
  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = ol_lyric_source_candidate_get_property;
  object_class->set_property = ol_lyric_source_candidate_set_property;
  object_class->finalize = ol_lyric_source_candidate_finalize;
  g_object_class_install_property (object_class,
                                   CANDIDATE_PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        ("Title"),
                                                        ("Title"),
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   CANDIDATE_PROP_ARTIST,
                                   g_param_spec_string ("artist",
                                                        ("Artist"),
                                                        ("Artist"),
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   CANDIDATE_PROP_ALBUM,
                                   g_param_spec_string ("album",
                                                        ("Album"),
                                                        ("Album"),
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   CANDIDATE_PROP_COMMENT,
                                   g_param_spec_string ("comment",
                                                        ("Comment"),
                                                        ("Comment"),
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   CANDIDATE_PROP_SOURCEID,
                                   g_param_spec_string ("sourceid",
                                                        ("SourceId"),
                                                        ("Source ID"),
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   CANDIDATE_PROP_DOWNLOADINFO,
                                   g_param_spec_variant ("downloadinfo",
                                                         ("Download Info"),
                                                         ("Download Info"),
                                                         G_VARIANT_TYPE_ANY,
                                                         NULL,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ol_lyric_source_candidate_init (OlLyricSourceCandidate *source)
{
  /* Allocate Private data structure */
  (OL_LYRIC_SOURCE_CANDIDATE(source))->priv = \
    (OlLyricSourceCandidatePrivate *) g_malloc0(sizeof(OlLyricSourceCandidatePrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_LYRIC_SOURCE_CANDIDATE(source))->priv != NULL)
  {
    OlLyricSourceCandidatePrivate *priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE(source);
    /* Initialize private data, if any */
  }
}

static void
ol_lyric_source_candidate_dispose(GObject *object)
{
  OlLyricSourceCandidate *self = (OlLyricSourceCandidate *)object;
  OlLyricSourceCandidatePrivate *priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE(self);
  /* Check if not NULL! To avoid calling dispose multiple times */
  if(priv != NULL)
  {
    /* Deallocate contents of the private data, if any */
    /* Deallocate private data structure */
    g_free(priv);
    /* And finally set the opaque pointer back to NULL, so that
     *  we don't deallocate it twice. */
    (OL_LYRIC_SOURCE_CANDIDATE(self))->priv = NULL;
  }
}

static OlLyricSourceCandidate *
ol_lyric_source_candidate_new (const gchar *title,
                               const gchar *artist,
                               const gchar *album,
                               const gchar *comment,
                               const gchar *sourceid,
                               GVariant *downloadinfo)
{
  ol_assert_ret (sourceid != NULL, NULL);
  ol_assert_ret (downloadinfo != NULL, NULL);
  return OL_LYRIC_SOURCE_CANDIDATE (g_object_new (OL_TYPE_LYRIC_SOURCE_CANDIDATE,
                                                  "title", title,
                                                  "artist", artist,
                                                  "album", album,
                                                  "comment", comment,
                                                  "sourceid", sourceid,
                                                  "downloadinfo", downloadinfo,
                                                  NULL));
}

static OlLyricSourceCandidate *
ol_lyric_source_candidate_new_with_variant (GVariant *dict)
{
  ol_log_func ();
  OlLyricSourceCandidate *candidate = NULL;
  GVariantIter *dictiter = NULL;
  gchar *key = NULL;
  gchar *title, *artist, *album, *comment, *sourceid;
  GVariant *downloadinfo = NULL;
  GVariant *value = NULL;
  title = artist = album = comment = sourceid = NULL;
  g_variant_get (dict, "a{sv}", &dictiter);
  while (g_variant_iter_loop (dictiter, "{sv}", &key, &value))
  {
    if (strcmp (key, "title") == 0 && title == NULL)
    {
      title = g_variant_dup_string (value, NULL);
    }
    else if (strcmp (key, "artist") == 0 && artist == NULL)
    {
      artist = g_variant_dup_string (value, NULL);
    }
    else if (strcmp (key, "album") == 0 && album == NULL)
    {
      album = g_variant_dup_string (value, NULL);
    }
    else if (strcmp (key, "comment") == 0 && comment == NULL)
    {
      comment = g_variant_dup_string (value, NULL);
    }
    else if (strcmp (key, "sourceid") == 0 && sourceid == NULL)
    {
      sourceid = g_variant_dup_string (value, NULL);
    }
    else if (strcmp (key, "downloadinfo") == 0 && downloadinfo == NULL)
    {
      downloadinfo = g_variant_ref (value);
    }
    else
    {
      ol_errorf ("Unknown candidate key: %s\n", key);
    }
  }
  g_variant_iter_free (dictiter);
  candidate = ol_lyric_source_candidate_new (title,
                                             artist,
                                             album,
                                             comment,
                                             sourceid,
                                             downloadinfo);
  g_free (title);
  g_free (artist);
  g_free (album);
  g_free (comment);
  g_free (sourceid);
  g_variant_unref (downloadinfo);
  return candidate;
}

static void
ol_lyric_source_candidate_finalize (GObject *object)
{
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (object);
  g_free (priv->title);
  g_free (priv->artist);
  g_free (priv->album);
  g_free (priv->sourceid);
  g_free (priv->comment);
  g_variant_unref (priv->downloadinfo);
  priv->title = NULL;
  priv->artist = NULL;
  priv->album = NULL;
  priv->downloadinfo = NULL;
  priv->comment = NULL;
  priv->sourceid = NULL;
  G_OBJECT_CLASS (ol_lyric_source_candidate_parent_class)->finalize (object);
}

static void
ol_lyric_source_candidate_get_property (GObject *object,
                                        guint property_id,
                                        GValue *value,
                                        GParamSpec *pspec)
{
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (object);
  switch (property_id)
  {
  case CANDIDATE_PROP_TITLE:
    g_value_set_string (value, priv->title);
    break;
  case CANDIDATE_PROP_ARTIST:
    g_value_set_string (value, priv->artist);
    break;
  case CANDIDATE_PROP_ALBUM:
    g_value_set_string (value, priv->album);
    break;
  case CANDIDATE_PROP_COMMENT:
    g_value_set_string (value, priv->comment);
    break;
  case CANDIDATE_PROP_SOURCEID:
    g_value_set_string (value, priv->sourceid);
    break;
  case CANDIDATE_PROP_DOWNLOADINFO:
    g_value_set_variant (value, priv->downloadinfo);
    break;
  }
}

static void
ol_lyric_source_candidate_set_property (GObject *object,
                                        guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec)
{
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (object);
  switch (property_id)
  {
  case CANDIDATE_PROP_TITLE:
    g_free (priv->title);
    priv->title = g_value_dup_string (value);
    break;
  case CANDIDATE_PROP_ARTIST:
    g_free (priv->artist);
    priv->artist = g_value_dup_string (value);
    break;
  case CANDIDATE_PROP_ALBUM:
    g_free (priv->album);
    priv->album = g_value_dup_string (value);
    break;
  case CANDIDATE_PROP_COMMENT:
    g_free (priv->comment);
    priv->comment = g_value_dup_string (value);
    break;
  case CANDIDATE_PROP_SOURCEID:
    g_free (priv->sourceid);
    priv->sourceid = g_value_dup_string (value);
    break;
  case CANDIDATE_PROP_DOWNLOADINFO:
    if (priv->downloadinfo != NULL)
      g_variant_unref (priv->downloadinfo);
    priv->downloadinfo = g_value_dup_variant (value);
    break;
  }
}

const gchar *
ol_lyric_source_candidate_get_title (OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (candidate);
  return priv->title;
}

const gchar *
ol_lyric_source_candidate_get_artist (OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (candidate);
  return priv->artist;
}

const gchar *
ol_lyric_source_candidate_get_album (OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (candidate);
  return priv->album;
}

const gchar *
ol_lyric_source_candidate_get_comment (OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (candidate);
  return priv->comment;
}

GVariant *
ol_lyric_source_candidate_get_downloadinfo (OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (candidate);
  return priv->downloadinfo;
}

const gchar *
ol_lyric_source_candidate_get_sourceid (OlLyricSourceCandidate *candidate)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_CANDIDATE (candidate), NULL);
  OlLyricSourceCandidatePrivate *priv;
  priv = OL_LYRIC_SOURCE_CANDIDATE_GET_PRIVATE (candidate);
  return priv->sourceid;
}

/* OlLyricSourceTask */
static void
ol_lyric_source_task_class_init (OlLyricSourceTaskClass *klass)
{
  GObjectClass *object_class;
  object_class = G_OBJECT_CLASS (klass);
  object_class->set_property = ol_lyric_source_task_set_property;
  object_class->get_property = ol_lyric_source_task_get_property;
  object_class->finalize = ol_lyric_source_task_finalize;
  klass->cancel = NULL;
  g_object_class_install_property (object_class,
                                   TASK_PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                        ("Source"),
                                                        ("Source"),
                                                        OL_TYPE_LYRIC_SOURCE,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
                                   TASK_PROP_TASKID,
                                   g_param_spec_int ("taskid",
                                                     ("Task ID"),
                                                     ("Task ID"),
                                                     0,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
ol_lyric_source_task_init (OlLyricSourceTask *task)
{
  /* Allocate Private data structure */
  (OL_LYRIC_SOURCE_TASK(task))->priv = \
    (OlLyricSourceTaskPrivate *) g_malloc0(sizeof(OlLyricSourceTaskPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_LYRIC_SOURCE_TASK(task))->priv != NULL)
  {
    OlLyricSourceTaskPrivate *priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE(task);
  }
}

static void
ol_lyric_source_task_dispose(GObject *object)
{
  OlLyricSourceTask *self = (OlLyricSourceTask *)object;
  OlLyricSourceTaskPrivate *priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE(self);
  /* Check if not NULL! To avoid calling dispose multiple times */
  if(priv != NULL)
  {
    /* Deallocate contents of the private data, if any */
    /* Deallocate private data structure */
    g_free(priv);
    /* And finally set the opaque pointer back to NULL, so that
     *  we don't deallocate it twice. */
    (OL_LYRIC_SOURCE_TASK(self))->priv = NULL;
  }
}

static void
ol_lyric_source_task_get_property (GObject *object,
                                   guint property_id,
                                   GValue *value,
                                   GParamSpec *pspec)
{
  OlLyricSourceTaskPrivate *priv;
  priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE (object);
  switch (property_id)
  {
  case TASK_PROP_SOURCE:
    g_value_set_object (value, priv->source);
    break;
  case TASK_PROP_TASKID:
    g_value_set_int (value, priv->taskid);
    break;
  }
}

static void
ol_lyric_source_task_set_property (GObject *object,
                                   guint property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
  OlLyricSourceTaskPrivate *priv;
  priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE (object);
  switch (property_id)
  {
  case TASK_PROP_SOURCE:
    priv->source = g_value_get_object (value);
    if (priv->source != NULL)
      g_object_weak_ref (G_OBJECT (priv->source),
                         (GWeakNotify) ol_lyric_source_task_weak_notify,
                         object);
    break;
  case TASK_PROP_TASKID:
    priv->taskid = g_value_get_int (value);
    break;
  }
}

static void
ol_lyric_source_task_finalize (GObject *object)
{
  OlLyricSourceTaskPrivate *priv;
  priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE (object);
  if (priv->source)
  {
    g_object_weak_unref (G_OBJECT (priv->source),
                         (GWeakNotify) ol_lyric_source_task_weak_notify,
                         object);
    priv->source = NULL;
  }
}

void
ol_lyric_source_task_cancel (OlLyricSourceTask *task)
{
  OlLyricSourceTaskClass *klass;
  klass = OL_LYRIC_SOURCE_TASK_CLASS (G_OBJECT_GET_CLASS (task));
  if (klass->cancel)
    klass->cancel (task);
}

gint
ol_lyric_source_task_get_id (OlLyricSourceTask *task)
{
  ol_assert_ret (OL_IS_LYRIC_SOURCE_TASK (task), -1);
  return OL_LYRIC_SOURCE_TASK_GET_PRIVATE (task)->taskid;
}

static void
ol_lyric_source_task_weak_notify (OlLyricSourceTask *task,
                                  OlLyricSource *source)
{
  OlLyricSourceTaskPrivate *priv;
  priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE (task);
  if (priv->source)
  {
    g_object_weak_unref (G_OBJECT (priv->source),
                         (GWeakNotify) ol_lyric_source_task_weak_notify,
                         source);
    priv->source = NULL;
  }
}

static void
ol_lyric_source_task_call_cancel (OlLyricSourceTask *task,
                                  const gchar *method)
{
  ol_assert (OL_IS_LYRIC_SOURCE_TASK (task));
  ol_assert (method != NULL);
  OlLyricSourceTaskPrivate *priv;
  GVariant *ret;
  priv = OL_LYRIC_SOURCE_TASK_GET_PRIVATE (task);
  if (!priv->source)
    return;
  GDBusProxy *proxy = G_DBUS_PROXY (priv->source);
  ret = g_dbus_proxy_call_sync (proxy,
                                method,
                                g_variant_new ("(i)", priv->taskid),
                                G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                -1,
                                NULL,
                                NULL);
  g_variant_unref (ret);
}

/* OlLyricSourceSearchTask */

static void
ol_lyric_source_search_task_class_init (OlLyricSourceSearchTaskClass *klass)
{
  OlLyricSourceTaskClass *task_class;
  task_class = OL_LYRIC_SOURCE_TASK_CLASS (klass);
  task_class->cancel = ol_lyric_source_search_task_cancel;
  _search_signals[SEARCH_SIGNAL_COMPLETE] =
    g_signal_new ("complete",
                  OL_TYPE_LYRIC_SOURCE_SEARCH_TASK,
                  G_SIGNAL_NO_HOOKS | G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                  0,            /* offset */
                  NULL,         /* accumulator */
                  NULL,         /* accu_data */
                  ol_marshal_VOID__ENUM_POINTER,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_INT,
                  G_TYPE_POINTER);
  _search_signals[SEARCH_SIGNAL_STARTED] =
    g_signal_new ("started",
                  OL_TYPE_LYRIC_SOURCE_SEARCH_TASK,
                  G_SIGNAL_NO_HOOKS | G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                  0,            /* offset */
                  NULL,         /* accumulator */
                  NULL,         /* accu_data */
                  ol_marshal_VOID__STRING_STRING,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_STRING,
                  G_TYPE_STRING);
}

static void
ol_lyric_source_search_task_init (OlLyricSourceSearchTask *task)
{
}

static OlLyricSourceSearchTask *
ol_lyric_source_search_task_new (OlLyricSource *source,
                                 gint taskid)
{
  GObject *obj;
  obj = g_object_new (OL_TYPE_LYRIC_SOURCE_SEARCH_TASK,
                      "source", source,
                      "taskid", taskid,
                      NULL);
  return OL_LYRIC_SOURCE_SEARCH_TASK (obj);
}

static void
ol_lyric_source_search_task_cancel (OlLyricSourceTask *task)
{
  ol_lyric_source_task_call_cancel (task,
                                    "CancelSearch");
}

/* OlLyricSourceDownloadTask */

static void
ol_lyric_source_download_task_class_init (OlLyricSourceDownloadTaskClass *klass)
{
  OlLyricSourceTaskClass *task_class;
  task_class = OL_LYRIC_SOURCE_TASK_CLASS (klass);
  task_class->cancel = ol_lyric_source_download_task_cancel;
  _download_signals[DOWNLOAD_SIGNAL_COMPLETE] =
    g_signal_new ("complete",
                  OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK,
                  G_SIGNAL_NO_HOOKS | G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                  0,            /* offset */
                  NULL,         /* accumulator */
                  NULL,         /* accu_data */
                  ol_marshal_VOID__ENUM_STRING_UINT,
                  G_TYPE_NONE,
                  3,
                  G_TYPE_INT,
                  G_TYPE_STRING,
                  G_TYPE_UINT);
}

static void
ol_lyric_source_download_task_init (OlLyricSourceDownloadTask *task)
{
}

static OlLyricSourceDownloadTask *
ol_lyric_source_download_task_new (OlLyricSource *source,
                                   gint taskid)
{
  GObject *obj;
  obj = g_object_new (OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK,
                      "source", source,
                      "taskid", taskid,
                      NULL);
  return OL_LYRIC_SOURCE_DOWNLOAD_TASK (obj);
}

static void
ol_lyric_source_download_task_cancel (OlLyricSourceTask *task)
{
  ol_lyric_source_task_call_cancel (task, "CancelDownload");
}
