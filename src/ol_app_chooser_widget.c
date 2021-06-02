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

#include <math.h>
#include <string.h>
#include "ol_app_chooser_widget.h"
#include "ol_debug.h"

static const char *DEFAULT_ICON_NAME = "media-playback-start";
static const int DEFAULT_N_COLUMN = 4;
static const int IMAGE_SIZE = 64;
static const int LABLE_WIDTH = 80;

enum _OlAppChooserWidgetSignals {
  APP_ACTIVATE_SIGNAL = 0,
  LAST_SIGNAL,
};

typedef struct _OlAppChooserWidgetPrivate OlAppChooserWidgetPrivate;
struct _OlAppChooserWidgetPrivate
{
  GPtrArray *app_list;
  guint n_columns;
};


static guint _signals[LAST_SIGNAL];

static void _remove_child (GtkWidget *widget, gpointer userdata);
static GtkWidget *_new_app_button (OlAppChooserWidget *chooser, guint index);
static void _app_activate (GtkWidget *button, GAppInfo *info);
static GtkWidget *_load_image_from_name (const char *icon_name);
static GtkWidget *_load_image_from_gicon (GIcon *icon);
static GtkWidget *_image_from_app_info (GAppInfo *app_info);
static void _calc_size (guint count, guint *n_rows, guint *n_columns);
static gint _app_info_cmp (GAppInfo **lhs, GAppInfo **rhs);
static void ol_app_chooser_widget_destroy (GtkObject *object);

G_DEFINE_TYPE_WITH_PRIVATE (OlAppChooserWidget,
                            ol_app_chooser_widget,
                            GTK_TYPE_TABLE);

static void
ol_app_chooser_widget_class_init (OlAppChooserWidgetClass *klass)
{
  _signals[APP_ACTIVATE_SIGNAL] =
    g_signal_new ("app-activate",
                  OL_TYPE_APP_CHOOSER_WIDGET,
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,            /* class_offset */
                  NULL,         /* accumulator */
                  NULL,         /* accumulator data */
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_APP_INFO);
}

static void
ol_app_chooser_widget_init (OlAppChooserWidget *chooser)
{
  OlAppChooserWidgetPrivate *priv = ol_app_chooser_widget_get_instance_private (chooser);
  priv->app_list = g_ptr_array_new_with_free_func (g_object_unref);
  priv->n_columns = -1;
}

static void
ol_app_chooser_widget_destroy (GtkObject *object)
{
  GtkObjectClass *parent_class = GTK_OBJECT_CLASS(ol_app_chooser_widget_parent_class) ;
  parent_class->destroy (object);
}

void
ol_app_chooser_widget_set_app_list (OlAppChooserWidget *chooser,
                                    GList *app_list,
                                    guint n_columns)
{
  ol_assert (OL_IS_APP_CHOOSER_WIDGET (chooser));
  OlAppChooserWidgetPrivate *priv = ol_app_chooser_widget_get_instance_private (chooser);
  if (priv->app_list->len > 0)
    g_ptr_array_remove_range (priv->app_list, 0, priv->app_list->len);
  GList *iter;
  for (iter = app_list; iter != NULL; iter = g_list_next (iter))
  {
    if (!G_IS_APP_INFO (iter->data))
    {
      ol_errorf ("The data of app_list must be GAppInfo\n");
      continue;
    }
    GAppInfo *info = iter->data;
    if (info != NULL && g_app_info_should_show (info))
      g_ptr_array_add (priv->app_list, g_object_ref (info));
  }
  g_ptr_array_sort (priv->app_list, (GCompareFunc) _app_info_cmp);
  guint n_rows = 0;
  _calc_size (priv->app_list->len, &n_rows, &n_columns);
  priv->n_columns = n_columns;
  gtk_container_foreach (GTK_CONTAINER (chooser),
                         _remove_child,
                         NULL);
  gtk_table_resize (GTK_TABLE (chooser), n_rows, n_columns);
  guint i, row, col;
  row = 0; col = 0;
  for (i = 0; i < priv->app_list->len; i++)
  {
    GtkWidget *app_button = _new_app_button (chooser, i);
    gtk_widget_show_all (app_button);
    gtk_table_attach (GTK_TABLE (chooser),
                      app_button,
                      col, col + 1, /* left, right */
                      row, row + 1, /* top, bottom */
                      GTK_EXPAND, 0,         /* x and y options */
                      0, 0);        /* x and y padding */
    row += (col + 1) / n_columns;
    col = (col + 1) % n_columns;
  }
}

static void
_remove_child (GtkWidget *widget, gpointer userdata)
{
  ol_assert (GTK_IS_WIDGET (widget));
  GtkWidget *parent = gtk_widget_get_parent (widget);
  ol_assert (parent != NULL);
  gtk_container_remove (GTK_CONTAINER (parent), widget);
}

static void
_calc_size (guint count, guint *n_rows, guint *n_columns)
{
  ol_assert (n_rows != NULL && n_columns != NULL);
  if (*n_columns == 0)
  {
    *n_columns = ceil (sqrt (count));
    if (*n_columns < DEFAULT_N_COLUMN)
      *n_columns = DEFAULT_N_COLUMN;
  }
  *n_rows = count / *n_columns;
  if (*n_rows == 0 || (*n_rows) * (*n_columns) < count)
    *n_rows = *n_rows + 1;
}

static GtkWidget *
_new_app_button (OlAppChooserWidget *chooser, guint index)
{
  OlAppChooserWidgetPrivate *priv = ol_app_chooser_widget_get_instance_private (chooser);
  GAppInfo *info = G_APP_INFO (g_ptr_array_index (priv->app_list, index));
  if (info == NULL)
    return NULL;
  GtkWidget *image = _image_from_app_info (info);

  GtkWidget *label = gtk_label_new (g_app_info_get_display_name (info));
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_size_request (label, LABLE_WIDTH, -1);

  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), image, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

  GtkWidget *frame = gtk_aspect_frame_new (NULL, 0.5, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  GtkWidget *button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text (button, g_app_info_get_display_name (info));
  gtk_container_add (GTK_CONTAINER (button), frame);

  g_signal_connect (button,
                    "clicked",
                    G_CALLBACK (_app_activate),
                    info);
  return button;
}

static void
_app_activate (GtkWidget *button, GAppInfo *info)
{
  ol_assert (GTK_IS_WIDGET (button));
  ol_assert (G_IS_APP_INFO (info));
  GtkWidget *parent = gtk_widget_get_parent (button);
  if (OL_IS_APP_CHOOSER_WIDGET (parent))
  {
    g_signal_emit (parent,
                   _signals[APP_ACTIVATE_SIGNAL],
                   0,
                   info);
  }
  else
  {
    ol_errorf ("Activating an app button, but its parent is not an OlAppChooserWidget.\n");
  }
}

static GtkWidget *
_load_image_from_name (const char *icon_name)
{
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  if (!gtk_icon_theme_has_icon (icon_theme, icon_name))
    icon_name = DEFAULT_ICON_NAME;
  return gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
}

static GtkWidget *
_load_image_from_gicon (GIcon *icon)
{
  GtkIconTheme *icon_theme = gtk_icon_theme_get_default ();
  GtkIconInfo *icon_info;
  if ((icon_info = gtk_icon_theme_lookup_by_gicon (icon_theme,
                                                   icon,
                                                   GTK_ICON_SIZE_BUTTON,
                                                   0)) != NULL)
  {
    gtk_icon_info_free (icon_info);
    return gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
  }
  else
  {
    return gtk_image_new_from_icon_name (DEFAULT_ICON_NAME, GTK_ICON_SIZE_BUTTON);
  }
}

static GtkWidget
*_image_from_app_info (GAppInfo *app_info)
{
  GIcon *icon = g_app_info_get_icon (app_info);
  GtkWidget *image = NULL;
  if (icon)
  {
    image = _load_image_from_gicon (icon);
  }
  else
  {
    image = _load_image_from_name (g_app_info_get_executable (app_info));
  }
  gtk_image_set_pixel_size (GTK_IMAGE (image), IMAGE_SIZE);
  return image;
}

GtkWidget *
ol_app_chooser_widget_new (void)
{
  return GTK_WIDGET (g_object_new (OL_TYPE_APP_CHOOSER_WIDGET, NULL));
}

static gint
_app_info_cmp (GAppInfo **lhs, GAppInfo **rhs)
{
  return strcasecmp (g_app_info_get_display_name (*lhs),
                     g_app_info_get_display_name (*rhs));
}

guint
ol_app_chooser_widget_get_columns (OlAppChooserWidget *chooser)
{
  ol_assert_ret (OL_IS_APP_CHOOSER_WIDGET (chooser), 0);
  OlAppChooserWidgetPrivate *priv = ol_app_chooser_widget_get_instance_private (chooser);
  return priv->n_columns;
}
