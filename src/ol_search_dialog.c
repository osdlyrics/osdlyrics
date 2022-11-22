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
#include "ol_search_dialog.h"
#include "ol_gui.h"
#include "ol_app.h"
#include "ol_config_proxy.h"
#include "ol_lyric_candidate_list.h"
#include "ol_lyric_source_list.h"
#include "ol_intl.h"
#include "ol_debug.h"

const char *MSG_SEARCHING = N_("Searching lyrics from %s...");
const char *MSG_NOT_FOUND = N_("Ooops, no lyric found :(");
const char *MSG_FOUND = N_("%d lyrics found :)");
const char *MSG_SEARCH_FAILURE = N_("Fail to search. Please check network connection");
const char *MSG_SEARCH_CANCELLED = N_("Cancelled");

struct
{
  GtkWidget *window;
  GtkWidget *candidates_panel;
  GtkEntry *title;
  GtkEntry *artist;
  GtkTreeView *list;
  GtkLabel *msg;
  GtkWidget *download;
  GtkWidget *engine;
} widgets = {0};

static OlLyricSourceSearchTask *search_task = NULL;
static OlMetadata *global_metadata = NULL;

gboolean ol_search_dialog_search_click (GtkWidget *widget,
                                        gpointer data);
gboolean ol_search_dialog_download_click (GtkWidget *widget,
                                          gpointer data);
gboolean ol_search_dialog_cancel_click (GtkWidget *widget,
                                        gpointer data);
static void internal_select_changed (GtkTreeSelection *selection,
                                     gpointer data);
static gboolean internal_init ();
static void ol_search_dialog_search_complete_cb (OlLyricSourceSearchTask *task,
                                                 enum OlLyricSourceStatus status,
                                                 GList *results,
                                                 gpointer userdata);
static void ol_search_dialog_search_started_cb (OlLyricSourceSearchTask *task,
                                                const gchar *sourceid,
                                                const gchar *sourcename,
                                                gpointer userdata);
static void ol_search_dialog_download_complete_cb (OlLyricSourceDownloadTask *task,
                                                   enum OlLyricSourceStatus status,
                                                   const gchar *content,
                                                   guint len,
                                                   gpointer userdata);

static void
internal_select_changed (GtkTreeSelection *selection, gpointer data)
{
  if (widgets.download != NULL)
    gtk_widget_set_sensitive (GTK_WIDGET (widgets.download),
                              gtk_tree_selection_get_selected (selection,
                                                               NULL,
                                                               NULL));
}

gboolean
ol_search_dialog_download_click (GtkWidget *widget,
                                 gpointer data)
{
  ol_log_func ();
  ol_assert_ret (widgets.list != NULL, FALSE);
  OlLyricSourceCandidate *candidate;
  candidate = ol_lyric_candidate_list_get_selected (widgets.list);
  if (candidate)
  {
    OlLyricSourceDownloadTask *task;
    task = ol_lyric_source_download (ol_app_get_lyric_source (),
                                     candidate);
    gtk_label_set_text (widgets.msg, _("Downloading..."));
    g_signal_connect (task,
                      "complete",
                      G_CALLBACK (ol_search_dialog_download_complete_cb),
                      ol_metadata_dup (global_metadata));
  }
  return TRUE;
}

gboolean
ol_search_dialog_cancel_click (GtkWidget *widget,
                               gpointer data)
{
  if (widgets.window != NULL)
    gtk_widget_hide (widgets.window);
  return TRUE;
}

gboolean
ol_search_dialog_search_click (GtkWidget *widget,
                               gpointer data)
{
  ol_log_func ();
  OlMetadata *metadata = ol_metadata_new ();
  ol_metadata_copy (metadata,
                    ol_app_get_current_music ());
  if (widgets.title != NULL)
    ol_metadata_set_title (metadata,
                           gtk_entry_get_text (widgets.title));
  if (widgets.artist != NULL)
    ol_metadata_set_artist (metadata,
                            gtk_entry_get_text (widgets.artist));
  gtk_widget_set_sensitive (GTK_WIDGET (widgets.list),
                            FALSE);
  gtk_widget_set_sensitive (widgets.download,
                            FALSE);
  GList *sourceids = ol_lyric_source_list_get_active_id_list (GTK_TREE_VIEW(widgets.engine));
  search_task = ol_lyric_source_search (ol_app_get_lyric_source (),
                                        metadata,
                                        sourceids);
  g_signal_connect (search_task,
                    "complete",
                    G_CALLBACK (ol_search_dialog_search_complete_cb),
                    NULL);
  g_signal_connect (search_task,
                    "started",
                    G_CALLBACK (ol_search_dialog_search_started_cb),
                    NULL);
  for (; sourceids; sourceids = g_list_delete_link (sourceids, sourceids))
  {
    g_free (sourceids->data);
  }
  return TRUE;
}

static void
ol_search_dialog_search_started_cb (OlLyricSourceSearchTask *task,
                                    const gchar *sourceid,
                                    const gchar *sourcename,
                                    gpointer userdata)
{
  if (widgets.msg != NULL && task == search_task)
  {
    char *msg = g_strdup_printf (_(MSG_SEARCHING),
                                 sourcename);
    gtk_label_set_text (widgets.msg, msg);
  }
}

static void
ol_search_dialog_search_complete_cb (OlLyricSourceSearchTask *task,
                                     enum OlLyricSourceStatus status,
                                     GList *results,
                                     gpointer userdata)
{
  ol_log_func ();
  if (task != search_task)
    return;
  gtk_widget_set_sensitive (GTK_WIDGET (widgets.list),
                            TRUE);
  ol_lyric_candidate_list_set_list (widgets.list,
                                  results);
  gtk_widget_show (widgets.candidates_panel);
  if (widgets.msg != NULL)
  {
    if (status == OL_LYRIC_SOURCE_STATUS_SUCCESS)
    {
      if (results != NULL)
      {
        int cnt = 0;
        GList *iter;
        for (iter = results; iter; iter = g_list_next (iter))
          cnt++;
        char *msg = g_strdup_printf (_(MSG_FOUND), cnt);
        gtk_label_set_text (widgets.msg, msg);
      }
      else
      {
        gtk_label_set_text (widgets.msg, _(MSG_NOT_FOUND));
      }
    }
    else if (status == OL_LYRIC_SOURCE_STATUS_CANCELLED)
    {
      gtk_label_set_text (widgets.msg, _(MSG_SEARCH_CANCELLED));
    }
    else
    {
      gtk_label_set_text (widgets.msg, _(MSG_SEARCH_FAILURE));
    }
  }
}

static void
ol_search_dialog_download_complete_cb (OlLyricSourceDownloadTask *task,
                                       enum OlLyricSourceStatus status,
                                       const gchar *content,
                                       guint len,
                                       gpointer userdata)
{
  OlMetadata *metadata = userdata;
  if (status == OL_LYRIC_SOURCE_STATUS_SUCCESS)
  {
    GError *error = NULL;
    gchar *uri = ol_lyrics_set_content (ol_app_get_lyrics_proxy (),
                                        metadata,
                                        content,
                                        &error);
    if (!uri)
    {
      ol_errorf ("Set content failed: %s\n",
                 error->message);
      g_error_free (error);
      gtk_label_set_text (widgets.msg, _("Download complete, but fail to assign to the track"));
    }
    else
    {
      gtk_label_set_text (widgets.msg, _("Download complete"));
      ol_debugf ("Set content to %s\n", uri);
      g_free (uri);
    }
  }
  else if (status == OL_LYRIC_SOURCE_STATUS_FALIURE)
  {
    gtk_label_set_text (widgets.msg, _("Fail to download lyric"));
  }
  ol_metadata_free (metadata);
}

static gboolean
internal_init ()
{
  if (widgets.window == NULL)
  {
    widgets.window = ol_gui_get_widget ("search-dialog");
    if (widgets.window == NULL)
    {
      ol_error ("Search Dialog not found");
      return FALSE;
    }
    g_signal_connect (G_OBJECT (widgets.window),
                      "delete-event",
                      G_CALLBACK (gtk_widget_hide_on_delete),
                      NULL);
    widgets.title = GTK_ENTRY (ol_gui_get_widget ("search-title"));
    widgets.artist = GTK_ENTRY (ol_gui_get_widget ("search-artist"));
    widgets.candidates_panel = ol_gui_get_widget ("search-candidates");
    widgets.list = GTK_TREE_VIEW (ol_gui_get_widget ("search-candidates-list"));
    widgets.msg = GTK_LABEL (ol_gui_get_widget ("search-msg"));
    widgets.download = ol_gui_get_widget ("search-download");
    ol_lyric_candidate_list_init (widgets.list,
                                G_CALLBACK (internal_select_changed));
    widgets.engine = ol_gui_get_widget ("search-engine");
    ol_lyric_source_list_init (GTK_TREE_VIEW (widgets.engine));
  }
  return widgets.window != NULL;
}

void
ol_search_dialog_show ()
{
  if (!internal_init ())
    return;
  if (gtk_widget_get_visible (widgets.window))
    return;
  ol_lyric_candidate_list_clear (widgets.list);

  if (global_metadata == NULL)
    global_metadata = ol_metadata_new ();
  ol_metadata_copy (global_metadata, ol_app_get_current_music ());
  gtk_entry_set_text (widgets.title,
                      ol_metadata_get_title (global_metadata));
  gtk_entry_set_text (widgets.artist,
                      ol_metadata_get_artist (global_metadata));
  gtk_widget_set_sensitive (widgets.download,
                            FALSE);
  gtk_label_set_text (widgets.msg, "");
  OlLyricSource *lyric_source = ol_app_get_lyric_source ();
  GList *info_list = ol_lyric_source_list_sources (lyric_source);
  ol_lyric_source_list_set_info_list (GTK_TREE_VIEW (widgets.engine),
                                      info_list);
  for (; info_list; info_list = g_list_delete_link (info_list, info_list))
  {
    ol_lyric_source_info_free (info_list->data);
  }
  gtk_widget_show (widgets.window);
}
