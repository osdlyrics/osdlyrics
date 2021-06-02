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
#ifndef _OL_OSD_TOOLBAR_H_
#define _OL_OSD_TOOLBAR_H_

#include <gtk/gtkbutton.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtktypeutils.h>
#include "ol_player.h"

#define OL_OSD_TOOLBAR(obj)                  G_TYPE_CHECK_INSTANCE_CAST (obj, ol_osd_toolbar_get_type (), OlOsdToolbar)
#define OL_OSD_TOOLBAR_CLASS(klass)          GTK_CHECK_CLASS_CAST (klass, ol_osd_toolbar_get_type (), OlOsdToolbarClass)
#define OL_IS_OSD_TOOLBAR(obj)               G_TYPE_CHECK_INSTANCE_TYPE (obj, ol_osd_toolbar_get_type ())
#define OL_OSD_TOOLBAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), ol_osd_toolbar_get_type (), OlOsdToolbarClass))

typedef struct _OlOsdToolbar OlOsdToolbar;
typedef struct _OlOsdToolbarPrivate OlOsdToolbarPrivate;
typedef struct _OlOsdToolbarClass OlOsdToolbarClass;

struct _OlOsdToolbar
{
  GtkAlignment alignment;
  GtkHBox *center_box;
  GtkButton *play_button;
  GtkButton *pause_button;
  GtkButton *prev_button;
  GtkButton *next_button;
  GtkButton *stop_button;
  gpointer priv; /** Private data pointer */
};

struct _OlOsdToolbarPrivate
{
};

struct _OlOsdToolbarClass
{
  GtkAlignmentClass parent_class;
};

GType ol_osd_toolbar_get_type (void);

GtkWidget *ol_osd_toolbar_new (void);
void ol_osd_toolbar_set_player (OlOsdToolbar *toolbar, OlPlayer *player);
void ol_osd_toolbar_set_status (OlOsdToolbar *toolbar, enum OlPlayerStatus status);

#endif /* _OL_OSD_TOOLBAR_H_ */
