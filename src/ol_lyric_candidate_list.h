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
#ifndef _OL_LYRIC_CANDIDATE_LIST_H_
#define _OL_LYRIC_CANDIDATE_LIST_H_

#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include "ol_lyric_source.h"

void ol_lyric_candidate_list_init (GtkTreeView *list,
                                   GCallback select_change_callback);
void ol_lyric_candidate_list_set_list (GtkTreeView *list,
                                     GList *candidates);
OlLyricSourceCandidate *ol_lyric_candidate_list_get_selected (GtkTreeView *list);
void ol_lyric_candidate_list_clear (GtkTreeView *list);

#endif /* _OL_LYRIC_CANDIDATE_LIST_H_ */
