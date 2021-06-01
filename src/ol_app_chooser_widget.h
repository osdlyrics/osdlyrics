/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011  Tiger Soldier
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

#ifndef _OL_APP_CHOOSER_WIDGET_H_
#define _OL_APP_CHOOSER_WIDGET_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktable.h>
#include <gtk/gtkicontheme.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkaspectframe.h>
#include <gtk/gtktypeutils.h>

typedef struct {
  GtkWidget widget;
  GtkTable parent;
} OlAppChooserWidget;

typedef struct {
  GtkWidgetClass widget_class;
  GtkTableClass parent_class;
} OlAppChooserWidgetClass;

#define OL_TYPE_APP_CHOOSER_WIDGET                                      \
  ol_app_chooser_widget_get_type ()
#define OL_APP_CHOOSER_WIDGET(obj)                                      \
  G_TYPE_CHECK_INSTANCE_CAST (obj, OL_TYPE_APP_CHOOSER_WIDGET, OlAppChooserWidget)
#define OL_APP_CHOOSER_WIDGET_CLASS(klass)                              \
  GTK_CHECK_CLASS_CAST (klass, OL_TYPE_APP_CHOOSER_WIDGET, OlAppChooserWidgetClass)
#define OL_IS_APP_CHOOSER_WIDGET(obj)               \
  G_TYPE_CHECK_INSTANCE_TYPE (obj, OL_TYPE_APP_CHOOSER_WIDGET)
#define OL_APP_CHOOSER_WIDGET_GET_CLASS(obj)                            \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              OL_TYPE_APP_CHOOSER_WIDGET,               \
                              OlAppChooserWidgetClass))

GType ol_app_chooser_widget_get_type (void);
GtkWidget *ol_app_chooser_widget_new (void);

/**
 * Sets the app infos to the app chooser.
 *
 * @param chooser
 * @param app_list A GList of GAppInfo*.
 * @param n_columns The number of columns to show the apps. If it is set to 0, the
 *                  number of columns will be calculated according to the number
 *                  of apps.
 */
void ol_app_chooser_widget_set_app_list (OlAppChooserWidget *chooser,
                                         GList *app_list,
                                         guint n_columns);

/**
 * Gets the number of columns of displayed apps.
 *
 * The number is guaranteed to be non-zero after setting the app_list.
 * @param chooser
 *
 * @return
 */
guint ol_app_chooser_widget_get_columns (OlAppChooserWidget *chooser);
#endif /* _OL_APP_CHOOSER_WIDGET_H_ */
