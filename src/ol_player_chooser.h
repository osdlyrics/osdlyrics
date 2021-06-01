/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011  Tiger Soldier <tigersoldier@gmail.com>
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
#ifndef _OL_PLAYER_CHOOSER_H_
#define _OL_PLAYER_CHOOSER_H_

#include <gtk/gtkdialog.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkobject.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkentrycompletion.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtktypeutils.h>

#define OL_PLAYER_CHOOSER(obj)                  G_TYPE_CHECK_INSTANCE_CAST (obj, ol_player_chooser_get_type (), OlPlayerChooser)
#define OL_PLAYER_CHOOSER_CLASS(klass)          GTK_CHECK_CLASS_CAST (klass, ol_player_chooser_get_type (), OlPlayerChooserClass)
#define OL_IS_PLAYER_CHOOSER(obj)               G_TYPE_CHECK_INSTANCE_TYPE (obj, ol_player_chooser_get_type ())
#define OL_PLAYER_CHOOSER_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), ol_player_chooser_get_type (), OlPlayerChooserClass))

enum OlPlayerChooserResponse {
  OL_PLAYER_CHOOSER_RESPONSE_LAUNCH,
};

enum OlPlayerChooserState {
  OL_PLAYER_CHOOSER_STATE_NO_PLAYER,
  OL_PLAYER_CHOOSER_STATE_CONNECTED,
  OL_PLAYER_CHOOSER_STATE_LAUNCH_FAIL,
};

typedef struct _OlPlayerChooser OlPlayerChooser;
typedef struct _OlPlayerChooserClass OlPlayerChooserClass;

struct _OlPlayerChooser
{
  GtkDialog parent;
  gpointer priv;
};

struct _OlPlayerChooserClass
{
  GtkDialogClass parent_class;
};

GType ol_player_chooser_get_type (void);

/**
 * Creates a new player chooser window.
 *
 * @param supported_players List of *GAppInfo.
 *
 * @return
 */
GtkWidget *ol_player_chooser_new (GList *supported_players);

void ol_player_chooser_set_info (OlPlayerChooser *window,
                                 const char *title,
                                 const char *description);

void ol_player_chooser_set_image_by_name (OlPlayerChooser *window,
                                          const char *icon_name);

void ol_player_chooser_set_image_by_gicon (OlPlayerChooser *window,
                                           GIcon *icon);

void ol_player_chooser_set_info_by_state (OlPlayerChooser *window,
                                          enum OlPlayerChooserState state);

#endif /* _OL_PLAYER_CHOOSER_H_ */
