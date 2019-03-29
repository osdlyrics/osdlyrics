/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2012  Tiger Soldier <tigersoldi@gmail.com>
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
#include "ol_lyric_source_list.h"
#include "ol_lyric_source.h"
#include "ol_debug.h"

static void _cell_toggled_cb (GtkCellRendererToggle *cell_renderer,
                              gchar *path,
                              gpointer user_data);

static void
_cell_toggled_cb (GtkCellRendererToggle *cell_renderer,
                  gchar *path,
                  gpointer user_data)
{
  GtkListStore *liststore = GTK_LIST_STORE (user_data);
  gboolean active = gtk_cell_renderer_toggle_get_active (cell_renderer);
  GtkTreeIter iter;
  gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (liststore), &iter, path);
  gtk_list_store_set (liststore, &iter, 0, !active, -1);
}

void
ol_lyric_source_list_init (GtkTreeView *list)
{
  ol_assert (list != NULL);
  ol_assert (GTK_IS_TREE_VIEW (list));
  GtkListStore *liststore = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING);
  gtk_tree_view_set_model (list, GTK_TREE_MODEL (liststore));
  GtkCellRenderer *check_renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (check_renderer),
                                      FALSE);
  g_signal_connect (G_OBJECT (check_renderer),
                    "toggled",
                    (GCallback) _cell_toggled_cb,
                    liststore);
  gtk_tree_view_insert_column_with_attributes (list,
                                               0,
                                               NULL,
                                               check_renderer,
                                               "active", 0,
                                               NULL);
  gtk_tree_view_insert_column_with_attributes (list,
                                               1,
                                               NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", 1,
                                               NULL);
}

GList *
ol_lyric_source_list_get_active_id_list (GtkTreeView *list)
{
  ol_assert_ret (list != NULL, NULL);
  ol_assert_ret (GTK_IS_TREE_VIEW (list), NULL);
  GtkTreeModel *liststore = gtk_tree_view_get_model (list);
  GtkTreeIter iter;
  gboolean valid;
  GList *id_list = NULL;
  for (valid = gtk_tree_model_get_iter_first (liststore, &iter);
       valid;
       valid = gtk_tree_model_iter_next (liststore, &iter))
  {
    gboolean enabled;
    gchar *id;
    gtk_tree_model_get (liststore, &iter,
                        0, &enabled,
                        2, &id,
                        -1);
    if (enabled)
      id_list = g_list_prepend (id_list, id);
    else
      g_free (id);
  }
  return g_list_reverse (id_list);
}

void
ol_lyric_source_list_set_info_list (GtkTreeView *list,
                                    GList *info_list)
{
  ol_assert (GTK_IS_TREE_VIEW (list));
  ol_log_func ();
  GtkListStore *liststore = GTK_LIST_STORE (gtk_tree_view_get_model (list));
  GList *info_iter;
  gtk_list_store_clear (GTK_LIST_STORE (liststore));
  for (info_iter = info_list;
       info_iter != NULL;
       info_iter = g_list_next (info_iter))
  {
    GtkTreeIter iter = {0};
    OlLyricSourceInfo *source_info = info_iter->data;
    gtk_list_store_append (liststore, &iter);
    gtk_list_store_set (liststore, &iter,
                        0, ol_lyric_source_info_get_enabled (source_info),
                        1, ol_lyric_source_info_get_name (source_info),
                        2, ol_lyric_source_info_get_id (source_info),
                        -1);
  }
}
