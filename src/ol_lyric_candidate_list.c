/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2012  Tiger Soldier
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
#include "ol_lyric_candidate_list.h"
#include "ol_intl.h"
#include "ol_debug.h"

enum {
  TITLE_COLUMN = 0,
  ARTIST_COLUMN,
  ALBUM_COLUMN,
  COMMENT_COLUMN,
  DATA_COLUMN,
  COLUMN_COUNT,
};

void
ol_lyric_candidate_list_init (GtkTreeView *list,
                            GCallback select_changed_callback)
{
  ol_assert (list != NULL);
  GtkTreeStore *store = gtk_tree_store_new (COLUMN_COUNT,    /* Total number of columns */
                                            G_TYPE_STRING,   /* Track title */
                                            G_TYPE_STRING,   /* Artist      */
                                            G_TYPE_STRING,   /* Album       */
                                            G_TYPE_STRING,   /* Comment */
                                            G_TYPE_OBJECT);
  
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Title"),
                                                     renderer,
                                                     "text", TITLE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (list, column);
  column = gtk_tree_view_column_new_with_attributes (_("Artist"),
                                                     renderer,
                                                     "text", ARTIST_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (list, column);
  column = gtk_tree_view_column_new_with_attributes (_("Album"),
                                                     renderer,
                                                     "text", ALBUM_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (list, column);
  gtk_tree_view_set_model (list, GTK_TREE_MODEL (store));

  GtkTreeSelection *select;
  select = gtk_tree_view_get_selection (list);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  if (select_changed_callback != NULL)
  {
    g_signal_connect (G_OBJECT (select), "changed",
                      G_CALLBACK (select_changed_callback),
                      NULL);
  }
}

void
ol_lyric_candidate_list_set_list (GtkTreeView *list,
                                  GList *candidates)
{
  ol_assert (list != NULL);
  GtkTreeIter iter;
  GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (list));
  gboolean first = TRUE;
  ol_lyric_candidate_list_clear (list);
  for (; candidates; candidates = g_list_next (candidates))
  {
    OlLyricSourceCandidate *candidate = OL_LYRIC_SOURCE_CANDIDATE (candidates->data);
    if (candidate == NULL) continue;
    gtk_tree_store_append (store, &iter, NULL);  /* Acquire a top-level iterator */
    gtk_tree_store_set (store, &iter,
                        TITLE_COLUMN, ol_lyric_source_candidate_get_title (candidate),
                        ARTIST_COLUMN, ol_lyric_source_candidate_get_artist (candidate),
                        ALBUM_COLUMN, ol_lyric_source_candidate_get_album (candidate),
                        COMMENT_COLUMN, ol_lyric_source_candidate_get_comment (candidate),
                        DATA_COLUMN, candidate,
                        -1);
    /* ol_debugf ("  url: %s\n", candidates[i].url); */
    /* Select the first item */
    if (first)
    {
      gtk_tree_selection_select_iter (gtk_tree_view_get_selection (list),
                                      &iter);
      first = FALSE;
    }
  }
}

OlLyricSourceCandidate *
ol_lyric_candidate_list_get_selected (GtkTreeView *list)
{
  ol_assert_ret (list != NULL, FALSE);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (list);
  if (selection != NULL)
  {
    GtkTreeIter iter;
    GtkTreeModel *model = NULL;
    OlLyricSourceCandidate *candidate = NULL;
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
      return NULL;
    gtk_tree_model_get (model, &iter, DATA_COLUMN, &candidate, -1);
    g_object_unref (candidate);
    return candidate;
  }
  else
  {
    return NULL;
  }
}

void
ol_lyric_candidate_list_clear (GtkTreeView *list)
{
  GtkTreeStore *store = GTK_TREE_STORE (gtk_tree_view_get_model (list));
  gtk_tree_store_clear (store);
}
