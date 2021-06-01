/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2010  Sarlmol Apple <sarlmolapple@gmail.com>
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
#ifndef __SCROLL_WINDOW_H_
#define __SCROLL_WINDOW_H_

#include <gtk/gtkwindow.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtktypeutils.h>
#include <ol_debug.h>
#include "ol_color.h"

#define OL_SCROLL_WINDOW(obj)                   G_TYPE_CHECK_INSTANCE_CAST (obj, ol_scroll_window_get_type (), OlScrollWindow)
#define OL_SCROLL_WINDOW_CLASS(klass)           GTK_CHECK_CLASS_CAST (klass, ol_scroll_window_get_type (), OlScrollWindowClass)
#define OL_IS_SCROLL_WINDOW(obj)                G_TYPE_CHECK_INSTANCE_TYPE (obj, ol_scroll_window_get_type ())
#define OL_SCROLL_WINDOW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), ol_scroll_window_get_type (), OlScrollWindowClass))

typedef struct _OlScrollWindow                  OlScrollWindow;
typedef struct _OlScrollWindowClass             OlScrollWindowClass;

enum OlScrollWindowScrollMode {
  OL_SCROLL_WINDOW_ALWAYS,
  OL_SCROLL_WINDOW_BY_LINES,
};

struct _OlScrollWindow
{
  /*basic*/
  GtkWindow widget;
  double percentage;
  GPtrArray *whole_lyrics;
  gint current_lyric_id;
  gpointer priv; /** Private data pointer */
};


struct _OlScrollWindowClass
{
  GtkWindowClass parent_class;
};

GType ol_scroll_window_get_type (void);

/**
 * @brief create a new Scroll Window.
 * To destroy the Scroll Window, use g_object_unref
 */

GtkWidget* ol_scroll_window_new (void);

/**
 * @brief Set the whole lyric of a song
 * If music changes,the whole lyrics of window will be changed.
 * @param scroll An OlScrollWindow
 * @param whole_lyrics The lyrics of a song. NULL means the line has no lyric
 *                     currently. The scroll window will increase the ref count of it.
 */
void ol_scroll_window_set_whole_lyrics(OlScrollWindow *scroll,
                                       GPtrArray *whole_lyrics);
/**
 * @brief Sets the progress of the lyrics
 * @param scroll An OlScrollWindow
 * @param lyric_id The lyric_line which is currenty being displayed. -1  means the line has no lyric currently.
 * @param percentage The width percentage of the left part whose color is changed
 */
void ol_scroll_window_set_progress (OlScrollWindow *scroll,
                                    int lyric_id,
                                    double percentage);


/**
 * @brief Gets the current line number
 * The current line is the lyric which is playing currently.
 * @param scroll An OlScrollWindow
 * @param line The line number of the current lyric, can be 0 or 1. 0 is the upper line and 1 is the lower
 */
int ol_scroll_window_get_current_lyric_id (OlScrollWindow *scroll);
/**
 * @brief Sets the font family for an SCROLL Window
 *
 * @param scroll An OlScrollWindow;
 * @param font_name Font family, must not be NULL. The font_name contains style and
 *        size information. Should be able to pass the value to
 *        pango_font_description_from_string()
 */
void ol_scroll_window_set_font_name (OlScrollWindow *scroll,
                                     const char *font_family);
/**
 * @brief Gets the font family for an SCROLL Window
 *
 * @param scroll An OlScrollWindow
 * @return The font name, see the comment of ol_scroll_window_set_font_name
 */
const char* ol_scroll_window_get_font_name (OlScrollWindow *scroll);

/**
 * Sets the text to be shown
 *
 * The text will be shown only if the lyrics are set to be NULL
 * @param scroll
 * @param text The text to be set, or NULL.
 */
void ol_scroll_window_set_text (OlScrollWindow *scroll,
                                const char *text);

/**
 * Sets the opacity of the background
 *
 * @param scroll
 * @param opacity The opacity of the background. 0 being fully transparent
 *                and 1 meansfully opaque.
 *
 */
void ol_scroll_window_set_bg_opacity (OlScrollWindow *scroll,
                                      double opacity);
double ol_scroll_window_get_bg_opacity (OlScrollWindow *scroll);

void ol_scroll_window_set_active_color (OlScrollWindow *scroll,
                                        OlColor color);
OlColor ol_scroll_window_get_active_color (OlScrollWindow *scroll);

void ol_scroll_window_set_inactive_color (OlScrollWindow *scroll,
                                          OlColor color);
OlColor ol_scroll_window_get_inactive_color (OlScrollWindow *scroll);

void ol_scroll_window_set_bg_color (OlScrollWindow *scroll,
                                          OlColor color);
OlColor ol_scroll_window_get_bg_color (OlScrollWindow *scroll);

void ol_scroll_window_add_toolbar (OlScrollWindow *scroll,
                                   GtkWidget	*widget);
void ol_scroll_window_remove_toolbar (OlScrollWindow *scroll,
                                      GtkWidget *widget);

void ol_scroll_window_set_scroll_mode (OlScrollWindow *scroll,
                                       enum OlScrollWindowScrollMode mode);

enum OlScrollWindowScrollMode ol_scroll_window_get_scroll_mode (OlScrollWindow *scroll);

void ol_scroll_window_set_can_seek (OlScrollWindow *scroll,
                                    gboolean can_seek);

gboolean ol_scroll_window_get_can_seek (OlScrollWindow *scroll);
#endif /* __OL_SCROLL_WINDOW_H__ */
