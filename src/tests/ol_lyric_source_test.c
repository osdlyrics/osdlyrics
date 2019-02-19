/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2012  Tiger Soldier <tigersoldi@gmail.com>
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

#include <stdio.h>
#include <glib.h>
#include "ol_lyric_source.h"
#include "ol_test_util.h"

OlLyricSource *source;
GMainLoop *loop;
gint task_cnt = 0;

static void
check_quit (void)
{
  if (task_cnt == 0)
    g_main_loop_quit (loop);
}

static void
test_list_sources (void)
{
  GList *list = ol_lyric_source_list_sources (source);
  for (; list != NULL; list = g_list_delete_link (list, list))
  {
    OlLyricSourceInfo *info = list->data;
    printf ("%s: %s\n",
            ol_lyric_source_info_get_id (info),
            ol_lyric_source_info_get_name (info));
    ol_lyric_source_info_free (info);
  }
}

static void
print_status (enum OlLyricSourceStatus status)
{
  switch (status)
  {
  case OL_LYRIC_SOURCE_STATUS_SUCCESS:
    printf ("success\n");
    break;
  case OL_LYRIC_SOURCE_STATUS_CANCELLED:
    printf ("cancelled\n");
    break;
  case OL_LYRIC_SOURCE_STATUS_FALIURE:
    printf ("failure\n");
    break;
  default:
    printf ("unknown: %d\n", status);
    break;
  }
}

static void
download_complete_cb (OlLyricSourceDownloadTask *task,
                      enum OlLyricSourceStatus status,
                      const gchar *content,
                      guint len,
                      gpointer userdata)
{
  printf ("download #%d complete: ",
          ol_lyric_source_task_get_id (OL_LYRIC_SOURCE_TASK (task)));
  print_status (status);
  ol_test_expect (status == (gsize)userdata);
  printf ("len: %u\n", len);
  printf ("%s\n", content);
  task_cnt--;
  check_quit ();
}
                      

static void
test_download (OlLyricSourceCandidate *candidate)
{
  OlLyricSourceDownloadTask *task;
  task_cnt++;
  task = ol_lyric_source_download (source, candidate);
  g_signal_connect (G_OBJECT (task),
                    "complete",
                    (GCallback) download_complete_cb,
                    (gpointer) OL_LYRIC_SOURCE_STATUS_SUCCESS);
}

static void
test_download_cancel (OlLyricSourceCandidate *candidate)
{
  OlLyricSourceDownloadTask *task;
  task_cnt++;
  task = ol_lyric_source_download (source, candidate);
  g_signal_connect (G_OBJECT (task),
                    "complete",
                    (GCallback) download_complete_cb,
                    (gpointer) OL_LYRIC_SOURCE_STATUS_CANCELLED);
  ol_lyric_source_task_cancel (OL_LYRIC_SOURCE_TASK (task));
}

static void
search_complete_cb (OlLyricSourceSearchTask *task,
                    enum OlLyricSourceStatus status,
                    GList *results,
                    gpointer userdata)
{
  printf ("search complete: ");
  print_status (status);
  ol_test_expect (status == (gsize)userdata);
  if (status == OL_LYRIC_SOURCE_STATUS_SUCCESS)
  {
    if (results)
    {
      test_download (OL_LYRIC_SOURCE_CANDIDATE (results->data));
      test_download_cancel (OL_LYRIC_SOURCE_CANDIDATE (results->data));
    }
    for (; results; results = results->next)
    {
      OlLyricSourceCandidate *candidate;
      candidate = OL_LYRIC_SOURCE_CANDIDATE (results->data);
      printf ("(%s)%s - %s - %s\n",
              ol_lyric_source_candidate_get_sourceid (candidate),
              ol_lyric_source_candidate_get_title (candidate),
              ol_lyric_source_candidate_get_artist (candidate),
              ol_lyric_source_candidate_get_album (candidate));
    }
  }
  task_cnt--;
  check_quit ();
}

static void
search_started_cb (OlLyricSourceSearchTask *task,
                   const gchar *sourceid,
                   const gchar *sourcename)
{
  printf ("Search started: (%s) %s\n", sourceid, sourcename);
}

static GList*
get_default_sourcid_list (void)
{
  GList *list = ol_lyric_source_list_sources (source);
  GList *ids = NULL;
  for (; list != NULL; list = g_list_delete_link (list, list))
  {
    OlLyricSourceInfo *info = list->data;
    if (ol_lyric_source_info_get_enabled (info))
    {
      ids = g_list_prepend (ids, g_strdup (ol_lyric_source_info_get_id (info)));
    }
    ol_lyric_source_info_free (info);
  }
  return g_list_reverse (ids);
}

static void
free_sourceid_list (GList *ids)
{
  for (; ids != NULL; ids = g_list_delete_link (ids, ids))
  {
    g_free (ids->data);
  }
}

static void
test_search (void)
{
  task_cnt++;
  OlMetadata *metadata = ol_metadata_new ();
  ol_metadata_set_title (metadata, "虫儿飞");
  GList *ids = get_default_sourcid_list ();
  OlLyricSourceSearchTask *task = ol_lyric_source_search (source,
                                                          metadata,
                                                          ids);
  g_signal_connect (G_OBJECT (task),
                    "complete",
                    (GCallback) search_complete_cb,
                    (gpointer) OL_LYRIC_SOURCE_STATUS_SUCCESS);
  g_signal_connect (G_OBJECT (task),
                    "started",
                    (GCallback) search_started_cb,
                    NULL);
  ol_metadata_free (metadata);
  free_sourceid_list (ids);
}

static void
test_search_default (void)
{
  task_cnt++;
  OlMetadata *metadata = ol_metadata_new ();
  ol_metadata_set_title (metadata, "Heal the world");
  OlLyricSourceSearchTask *task = ol_lyric_source_search_default (source,
                                                                  metadata);
  g_signal_connect (G_OBJECT (task),
                    "complete",
                    (GCallback) search_complete_cb,
                    (gpointer) OL_LYRIC_SOURCE_STATUS_SUCCESS);
  g_signal_connect (G_OBJECT (task),
                    "started",
                    (GCallback) search_started_cb,
                    NULL);
  ol_metadata_free (metadata);
}

static void
test_search_cancel (void)
{
  task_cnt++;
  OlMetadata *metadata = ol_metadata_new ();
  ol_metadata_set_title (metadata, "放弃");
  GList *ids = get_default_sourcid_list ();
  OlLyricSourceSearchTask *task = ol_lyric_source_search (source,
                                                          metadata,
                                                          ids);
  g_signal_connect (G_OBJECT (task),
                    "complete",
                    (GCallback) search_complete_cb,
                    (gpointer) OL_LYRIC_SOURCE_STATUS_CANCELLED);
  g_signal_connect (G_OBJECT (task),
                    "started",
                    (GCallback) search_started_cb,
                    NULL);
  ol_lyric_source_task_cancel (OL_LYRIC_SOURCE_TASK (task));
  ol_metadata_free (metadata);
  free_sourceid_list (ids);
}

static void
test_empty_source (void)
{
  task_cnt++;
  OlMetadata *metadata = ol_metadata_new ();
  ol_metadata_set_title (metadata, "empty");
  OlLyricSourceSearchTask *task = ol_lyric_source_search (source,
                                                          metadata,
                                                          NULL);
  g_signal_connect (G_OBJECT (task),
                    "complete",
                    (GCallback) search_complete_cb,
                    (gpointer)OL_LYRIC_SOURCE_STATUS_FALIURE);
  g_signal_connect (G_OBJECT (task),
                    "started",
                    (GCallback) search_started_cb,
                    NULL);
  ol_lyric_source_task_cancel (OL_LYRIC_SOURCE_TASK (task));
  ol_metadata_free (metadata);
}

int
main (int argc, char **argv)
{
  source = ol_lyric_source_new ();
  test_list_sources ();
  test_search ();
  test_search_default ();
  test_search_cancel ();
  test_empty_source ();
  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_object_unref (source);
  return 0;
}
