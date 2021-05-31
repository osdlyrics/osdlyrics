/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2012  Tiger Soldier <tigersoldier@gmail.com>
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
#ifndef _OL_LYRIC_SOURCE_LIST_H_
#define _OL_LYRIC_SOURCE_LIST_H_

#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtkcellrenderertext.h>

void ol_lyric_source_list_init (GtkTreeView *list);

/**
 * Sets source list with source info
 *
 * The sources will be listed in the order of the position in the list.
 * @param list
 * @param info_list A list of OlLyricSourceInfo*.
 */
void ol_lyric_source_list_set_info_list (GtkTreeView *list,
                                         GList *info_list);
/**
 * Gets a list of id of enabled sources
 *
 * The returned list can be passed as the `source_ids` parameter in
 * ol_lyric_source_search().
 *
 * @param list The engine list widget
 *
 * @return A list of gchar*. The elements in the list are id of enabled
 * sources. You must free the elemenets with g_free(), and free the list.
 */
GList *ol_lyric_source_list_get_active_id_list (GtkTreeView *list);

#endif /* _OL_LYRIC_SOURCE_LIST_H_ */
