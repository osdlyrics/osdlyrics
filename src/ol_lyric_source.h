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

#ifndef _OL_LYRIC_SOURCE_H_
#define _OL_LYRIC_SOURCE_H_

#include <gio/gio.h>
#include "ol_metadata.h"

/* OlLyricSource */
#define OL_TYPE_LYRIC_SOURCE                    \
  (ol_lyric_source_get_type ())
#define OL_LYRIC_SOURCE(obj)                                            \
  (G_TYPE_CHECK_INSTANCE_CAST (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE,                    \
                               OlLyricSource))
#define OL_LYRIC_SOURCE_CLASS(klass)                                    \
  (G_TYPE_CHECK_CLASS_CAST (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE,                       \
                            OlLyricSourceClass))
#define OL_IS_LYRIC_SOURCE(obj)                             \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj,                         \
                               OL_TYPE_LYRIC_SOURCE))
#define OL_IS_LYRIC_SOURCE_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_TYPE (klass,                        \
                            OL_TYPE_LYRIC_SOURCE))
#define OL_LYRIC_SOURCE_GET_CLASS(obj)                                  \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              OL_TYPE_LYRIC_SOURCE,                     \
                              OlLyricSourceClass))

/* OlLyricSourceTask */
#define OL_TYPE_LYRIC_SOURCE_TASK               \
  (ol_lyric_source_task_get_type ())
#define OL_LYRIC_SOURCE_TASK(obj)                                       \
  (G_TYPE_CHECK_INSTANCE_CAST (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE_TASK,               \
                               OlLyricSourceTask))
#define OL_LYRIC_SOURCE_TASK_CLASS(klass)                               \
  (G_TYPE_CHECK_CLASS_CAST (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE_TASK,                  \
                            OlLyricSourceTaskClass))
#define OL_IS_LYRIC_SOURCE_TASK(obj)                            \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj,                             \
                               OL_TYPE_LYRIC_SOURCE_TASK))
#define OL_IS_LYRIC_SOURCE_TASK_CLASS(klass)                    \
  (G_TYPE_CHECK_CLASS_TYPE (klass,                              \
                            OL_TYPE_LYRIC_SOURCE_TASK))
#define OL_LYRIC_SOURCE_TASK_GET_CLASS(obj)                             \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              OL_TYPE_LYRIC_SOURCE_TASK,                \
                              OlLyricSourceTaskClass))

/* OlLyricSourceSearchTask */
#define OL_TYPE_LYRIC_SOURCE_SEARCH_TASK               \
  (ol_lyric_source_search_task_get_type ())
#define OL_LYRIC_SOURCE_SEARCH_TASK(obj)                                \
  (G_TYPE_CHECK_INSTANCE_CAST (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE_SEARCH_TASK,        \
                               OlLyricSourceSearchTask))
#define OL_LYRIC_SOURCE_SEARCH_TASK_CLASS(klass)                        \
  (G_TYPE_CHECK_CLASS_CAST (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE_SEARCH_TASK,           \
                            OlLyricSourceSearchTaskClass))
#define OL_IS_LYRIC_SOURCE_SEARCH_TASK(obj)                            \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj,                                    \
                               OL_TYPE_LYRIC_SOURCE_SEARCH_TASK))
#define OL_IS_LYRIC_SOURCE_SEARCH_TASK_CLASS(klass)                    \
  (G_TYPE_CHECK_CLASS_TYPE (klass,                                     \
                            OL_TYPE_LYRIC_SOURCE_SEARCH_TASK))
#define OL_LYRIC_SOURCE_SEARCH_TASK_GET_CLASS(obj)                      \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              OL_TYPE_LYRIC_SOURCE_SEARCH_TASK,         \
                              OlLyricSourceSearchTaskClass))

/* OlLyricSourceDownloadTask */
#define OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK               \
  (ol_lyric_source_download_task_get_type ())
#define OL_LYRIC_SOURCE_DOWNLOAD_TASK(obj)                              \
  (G_TYPE_CHECK_INSTANCE_CAST (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK,      \
                               OlLyricSourceDownloadTask))
#define OL_LYRIC_SOURCE_DOWNLOAD_TASK_CLASS(klass)                      \
  (G_TYPE_CHECK_CLASS_CAST (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK,         \
                            OlLyricSourceDownloadTaskClass))
#define OL_IS_LYRIC_SOURCE_DOWNLOAD_TASK(obj)                           \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK))
#define OL_IS_LYRIC_SOURCE_DOWNLOAD_TASK_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_TYPE (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK))
#define OL_LYRIC_SOURCE_DOWNLOAD_TASK_GET_CLASS(obj)                    \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              OL_TYPE_LYRIC_SOURCE_DOWNLOAD_TASK,       \
                              OlLyricSourceDownloadTaskClass))

/* OlLyricSourceCandidate */
#define OL_TYPE_LYRIC_SOURCE_CANDIDATE               \
  (ol_lyric_source_candidate_get_type ())
#define OL_LYRIC_SOURCE_CANDIDATE(obj)                              \
  (G_TYPE_CHECK_INSTANCE_CAST (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE_CANDIDATE,      \
                               OlLyricSourceCandidate))
#define OL_LYRIC_SOURCE_CANDIDATE_CLASS(klass)                      \
  (G_TYPE_CHECK_CLASS_CAST (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE_CANDIDATE,         \
                            OlLyricSourceCandidateClass))
#define OL_IS_LYRIC_SOURCE_CANDIDATE(obj)                           \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj,                                     \
                               OL_TYPE_LYRIC_SOURCE_CANDIDATE))
#define OL_IS_LYRIC_SOURCE_CANDIDATE_CLASS(klass)                   \
  (G_TYPE_CHECK_CLASS_TYPE (klass,                                      \
                            OL_TYPE_LYRIC_SOURCE_CANDIDATE))
#define OL_LYRIC_SOURCE_CANDIDATE_GET_CLASS(obj)                    \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              OL_TYPE_LYRIC_SOURCE_CANDIDATE,       \
                              OlLyricSourceCandidateClass))

enum OlLyricSourceStatus {
  OL_LYRIC_SOURCE_STATUS_SUCCESS = 0,
  OL_LYRIC_SOURCE_STATUS_CANCELLED = 1,
  OL_LYRIC_SOURCE_STATUS_FALIURE = 2,
};

typedef struct _OlLyricSource OlLyricSource;
typedef struct _OlLyricSourceClass OlLyricSourceClass;

typedef struct _OlLyricSourceTask OlLyricSourceTask;
typedef struct _OlLyricSourceTaskClass OlLyricSourceTaskClass;

typedef struct _OlLyricSourceSearchTask OlLyricSourceSearchTask;
typedef struct _OlLyricSourceSearchTaskClass OlLyricSourceSearchTaskClass;

typedef struct _OlLyricSourceDownloadTask OlLyricSourceDownloadTask;
typedef struct _OlLyricSourceDownloadTaskClass OlLyricSourceDownloadTaskClass;

typedef struct _OlLyricSourceCandidate OlLyricSourceCandidate;
typedef struct _OlLyricSourceCandidateClass OlLyricSourceCandidateClass;

typedef struct _OlLyricSourceInfo OlLyricSourceInfo;

struct _OlLyricSourceInfo;

/**
 * Prototype of callback function of 'complete' signal in
 * OlLyricSourceSearchTask.
 *
 * @param task The task object that emits this signal.
 * @param status The status of the search task.
 * @param results A list of OlLyricSourceCandidate* objects.
 * @param userdata
 */
typedef void (*OlSearchCompleteFunc) (OlLyricSourceSearchTask *task,
                                      enum OlLyricSourceStatus status,
                                      GList *results,
                                      gpointer userdata);
typedef void (*OlSearchStartedFunc) (OlLyricSourceSearchTask *task,
                                     const gchar *sourceid,
                                     const gchar *sourcename,
                                     gpointer userdata);

/**
 * Prototype of callback function of 'complete' signal in
 * OlLyricSourceDownloadTask.
 *
 * @param task The task that emits this signal.
 * @param status The status of the download task.
 * @param content If status is OL_LYRIC_SOURCE_STATUS_SUCCESS, this
 *        value is the content of the downloaded lyric file.
 * @param len The length of the content.
 * @param userdata
 */
typedef void (*OlDownloadCompleteFunc) (OlLyricSourceDownloadTask *task,
                                        enum OlLyricSourceStatus status,
                                        const gchar *content,
                                        guint len,
                                        gpointer userdata);

struct _OlLyricSourceTask
{
  GObject parent;
  gpointer priv;
};

struct _OlLyricSourceTaskClass
{
  GObjectClass parent_class;
  void (*cancel) (OlLyricSourceTask *task);
};

struct _OlLyricSourceSearchTask
{
  OlLyricSourceTask parent;
};

struct _OlLyricSourceSearchTaskClass
{
  OlLyricSourceTaskClass parent_class;
};

struct _OlLyricSourceDownloadTask
{
  OlLyricSourceTask parent;
};

struct _OlLyricSourceDownloadTaskClass
{
  OlLyricSourceTaskClass parent_class;
};

struct _OlLyricSource
{
  GDBusProxy parent;
  gpointer priv;
};

struct _OlLyricSourceClass
{
  GDBusProxyClass parent_class;
  gpointer priv;
};

struct _OlLyricSourceCandidate
{
  GObject parent;
  gpointer priv;
};

struct _OlLyricSourceCandidateClass
{
  GObjectClass parent_class;
};

/* OlLyricSource */
/**
 * Creates a new lyric source proxy
 *
 */
OlLyricSource *ol_lyric_source_new (void);
GType ol_lyric_source_get_type (void);
/**
 * Lists all the available sources.
 *
 * @param source The lyric source proxy.
 *
 * @return A list of `OlLyricSourceInfo*`. Caller should free this list and its
 * elements.
 */
GList *ol_lyric_source_list_sources (OlLyricSource* source);
/**
 * Search lyrics for a given metadata.
 *
 * @param source The lyric source proxy.
 * @param metadata The metadata to be searched for.
 * @param source_ids A list of gchar *. The ID of lyric sources.
 *
 * @return An OlLyricSourceSearchTask object of the search task. The
 * lyric source proxy owns a reference of it. If you want to take a
 * reference to the returned object, use g_object_ref().
 */
OlLyricSourceSearchTask *ol_lyric_source_search (OlLyricSource *source,
                                                 OlMetadata *metadata,
                                                 GList *source_ids);
/**
 * Search for lyrics with default lyric sources.
 *
 * @param source The lyric source proxy.
 * @param metadata The metadata to be searched for.
 *
 * @return An OlLyricSourceSearchTask object of the search task. The
 * lyric source proxy owns a reference of it. If you want to take a
 * reference to the returned object, use g_object_ref().
 */
OlLyricSourceSearchTask *ol_lyric_source_search_default (OlLyricSource *source,
                                                         OlMetadata *metadata);
/**
 * Download a lyric file provided by a candidate.
 *
 * @param source The lyric source proxy.
 * @param candidate The candidate.
 *
 * @return An OlLyricSourceDownloadTask object of the search task. If
 * you want to take a reference to the returned object, use
 * g_object_ref().
 */
OlLyricSourceDownloadTask *ol_lyric_source_download (OlLyricSource *source,
                                                     OlLyricSourceCandidate *candidate);

/* OlLyricSourceInfo */
const gchar *ol_lyric_source_info_get_id (OlLyricSourceInfo *info);
const gchar *ol_lyric_source_info_get_name (OlLyricSourceInfo *info);
gboolean ol_lyric_source_info_get_enabled (OlLyricSourceInfo *info);
void ol_lyric_source_info_free (OlLyricSourceInfo *info);
/* OlLyricSourceTask */
GType ol_lyric_source_task_get_type (void);
gint ol_lyric_source_task_get_id (OlLyricSourceTask *task);
void ol_lyric_source_task_cancel (OlLyricSourceTask *task);

/* OlLyricSourceSearchTask */
GType ol_lyric_source_search_task_get_type (void);

/* OlLyricSourceDownloadTask */
GType ol_lyric_source_get_type (void);

/* OlLyricSourceCandidate */
GType ol_lyric_source_candidate_get_type (void);
const gchar *ol_lyric_source_candidate_get_title (OlLyricSourceCandidate *candidate);
const gchar *ol_lyric_source_candidate_get_artist (OlLyricSourceCandidate *candidate);
const gchar *ol_lyric_source_candidate_get_album (OlLyricSourceCandidate *candidate);
const gchar *ol_lyric_source_candidate_get_comment (OlLyricSourceCandidate *candidate);
const gchar *ol_lyric_source_candidate_get_sourceid (OlLyricSourceCandidate *candidate);

GVariant *ol_lyric_source_candidate_get_downloadinfo (OlLyricSourceCandidate *candidate);

#endif /* _OL_LYRIC_SOURCE_H_ */
