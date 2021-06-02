/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldi@gmail.com>
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
#include "ol_osd_toolbar.h"
#include "ol_image_button.h"
#include "ol_stock.h"
#include "ol_debug.h"

#define OL_OSD_TOOLBAR_GET_PRIVATE(obj) \
    ((OlOsdToolbarPriv *)((OL_OSD_TOOLBAR(obj))->priv))

typedef struct _OlOsdToolbarPriv OlOsdToolbarPriv;
struct _OlOsdToolbarPriv
{
  OlPlayer *player;
  enum OlPlayerStatus status;
};

G_DEFINE_TYPE_WITH_PRIVATE (OlOsdToolbar, ol_osd_toolbar, GTK_TYPE_ALIGNMENT);

enum {
  BTN_PLAY = 0,
  BTN_PAUSE,
  BTN_STOP,
  BTN_PREV,
  BTN_NEXT,
};

struct ButtonSpec
{
  const char *stock;
  void (*handler) (GtkButton *button, OlOsdToolbar *toolbar);
};

static void _player_control (OlPlayer *player,
                             enum OlPlayerCaps caps,
                             gboolean (*cmd) (OlPlayer *player));
static void _play_clicked (GtkButton *button, OlOsdToolbar *toolbar);
static void _pause_clicked (GtkButton *button, OlOsdToolbar *toolbar);
static void _stop_clicked (GtkButton *button, OlOsdToolbar *toolbar);
static void _prev_clicked (GtkButton *button, OlOsdToolbar *toolbar);
static void _next_clicked (GtkButton *button, OlOsdToolbar *toolbar);
static GtkButton *_add_button (OlOsdToolbar *toolbar,
                               const struct ButtonSpec *btn_spec);
static void _update_caps (OlOsdToolbar *toolbar);
static void _update_status (OlOsdToolbar *toolbar);
static void _caps_changed_cb (OlPlayer *player,
                              OlOsdToolbar *toolbar);
static void _status_changed_cb (OlPlayer *player,
                                OlOsdToolbar *toolbar);
static void ol_osd_toolbar_destroy (GtkObject *obj);

const static struct ButtonSpec btn_spec[] = {
  {OL_STOCK_OSD_PLAY, _play_clicked},
  {OL_STOCK_OSD_PAUSE, _pause_clicked},
  {OL_STOCK_OSD_STOP, _stop_clicked},
  {OL_STOCK_OSD_PREV, _prev_clicked},
  {OL_STOCK_OSD_NEXT, _next_clicked},
};

static void
_player_control (OlPlayer *player,
                 enum OlPlayerCaps caps,
                 gboolean (*cmd) (OlPlayer *player))
{
  if (player != NULL &&
      (ol_player_get_caps (player) & caps))
  {
    cmd (player);
  }
}

static void
_play_clicked (GtkButton *button, OlOsdToolbar *toolbar)
{
  ol_assert (toolbar != NULL);
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  _player_control (priv->player,
                   OL_PLAYER_PLAY,
                   ol_player_play);
}

static void
_pause_clicked (GtkButton *button, OlOsdToolbar *toolbar)
{
  ol_assert (toolbar != NULL);
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  _player_control (priv->player,
                   OL_PLAYER_PAUSE,
                   ol_player_pause);
}

static void
_stop_clicked (GtkButton *button, OlOsdToolbar *toolbar)
{
  ol_assert (toolbar != NULL);
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  _player_control (priv->player,
                   OL_PLAYER_STOP,
                   ol_player_stop);
}

static void
_prev_clicked (GtkButton *button, OlOsdToolbar *toolbar)
{
  ol_assert (toolbar != NULL);
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  _player_control (priv->player,
                   OL_PLAYER_PREV,
                   ol_player_prev);
}

static void
_next_clicked (GtkButton *button, OlOsdToolbar *toolbar)
{
  ol_assert (toolbar != NULL);
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  _player_control (priv->player,
                   OL_PLAYER_NEXT,
                   ol_player_next);
}

static GtkButton *
_add_button (OlOsdToolbar *toolbar,
             const struct ButtonSpec *btn_spec)
{
  OlImageButton *btn = OL_IMAGE_BUTTON (ol_image_button_new ());
  GtkIconTheme *icontheme = gtk_icon_theme_get_default ();
  GtkIconInfo *info = gtk_icon_theme_lookup_icon (icontheme,
                                                  btn_spec->stock,
                                                  16,
                                                  0);

  GdkPixbuf *image = gdk_pixbuf_new_from_file (gtk_icon_info_get_filename (info),
                                               NULL);
  gtk_icon_info_free (info);
  ol_image_button_set_pixbuf (btn, image);
  g_signal_connect (btn,
                    "clicked",
                    G_CALLBACK (btn_spec->handler),
                    toolbar);
  gtk_box_pack_start (GTK_BOX (toolbar->center_box), GTK_WIDGET (btn),
                      FALSE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (btn));
  return GTK_BUTTON (btn);
}

static void
_caps_changed_cb (OlPlayer *player,
                  OlOsdToolbar *toolbar)
{
  _update_caps (toolbar);
}

static void
_status_changed_cb (OlPlayer *player,
                    OlOsdToolbar *toolbar)
{
  _update_status (toolbar);
}

static void
_update_caps (OlOsdToolbar *toolbar)
{
  ol_assert (OL_IS_OSD_TOOLBAR (toolbar));
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  int caps;
  if (priv->player)
    caps = ol_player_get_caps (priv->player);
  else
    caps = 0;
  gtk_widget_set_sensitive (GTK_WIDGET (toolbar->play_button),
                            caps & OL_PLAYER_PLAY);
  gtk_widget_set_sensitive (GTK_WIDGET (toolbar->pause_button),
                            caps & OL_PLAYER_PAUSE);
  gtk_widget_set_sensitive (GTK_WIDGET (toolbar->stop_button),
                            caps & OL_PLAYER_STOP);
  gtk_widget_set_sensitive (GTK_WIDGET (toolbar->prev_button),
                            caps & OL_PLAYER_PREV);
  gtk_widget_set_sensitive (GTK_WIDGET (toolbar->next_button),
                            caps & OL_PLAYER_NEXT);
}

static void
_update_status (OlOsdToolbar *toolbar)
{
  ol_log_func ();
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  enum OlPlayerStatus status;
  if (priv->player)
    status = ol_player_get_status (priv->player);
  else
    status = OL_PLAYER_UNKNOWN;
  switch (status)
  {
  case OL_PLAYER_PLAYING:
    gtk_widget_show (GTK_WIDGET (toolbar->pause_button));
    gtk_widget_hide (GTK_WIDGET (toolbar->play_button));
    break;
  case OL_PLAYER_PAUSED:
  case OL_PLAYER_STOPPED:
    gtk_widget_hide (GTK_WIDGET (toolbar->pause_button));
    gtk_widget_show (GTK_WIDGET (toolbar->play_button));
    break;
  default:
    gtk_widget_show (GTK_WIDGET (toolbar->pause_button));
    gtk_widget_show (GTK_WIDGET (toolbar->play_button));
    break;
  }
  gtk_widget_queue_draw (GTK_WIDGET (toolbar));
}

static void
ol_osd_toolbar_class_init (OlOsdToolbarClass *klass)
{
  GtkObjectClass *gtk_class = GTK_OBJECT_CLASS (klass);
  gtk_class->destroy = ol_osd_toolbar_destroy;
}

static void
ol_osd_toolbar_init (OlOsdToolbar *toolbar)
{
  /* Allocate Private data structure */
  (OL_OSD_TOOLBAR(toolbar))->priv = \
     (OlOsdToolbarPriv *) g_malloc0(sizeof(OlOsdToolbarPriv));
  /* If correctly allocated, initialize parameters */
  if((OL_OSD_TOOLBAR(toolbar))->priv != NULL)
  {
    OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
    gtk_alignment_set (GTK_ALIGNMENT (toolbar), 0.5, 0.5, 0.0, 0.0);
    toolbar->center_box = GTK_HBOX (gtk_hbox_new (FALSE, 0));
    gtk_container_add (GTK_CONTAINER (toolbar), GTK_WIDGET (toolbar->center_box));

    toolbar->prev_button = _add_button (toolbar, &btn_spec[BTN_PREV]);
    toolbar->play_button = _add_button (toolbar, &btn_spec[BTN_PLAY]);
    toolbar->pause_button = _add_button (toolbar, &btn_spec[BTN_PAUSE]);
    toolbar->stop_button = _add_button (toolbar, &btn_spec[BTN_STOP]);
    toolbar->next_button = _add_button (toolbar, &btn_spec[BTN_NEXT]);

    priv->player = NULL;
    _update_status (toolbar);
    _update_caps (toolbar);
  }
}

static void
my_gobject_dispose(GObject *object)
{
    OlOsdToolbar *toolbar = (OlOsdToolbar *)object;
    OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
    /* Check if not NULL! To avoid calling dispose multiple times */
    if(priv != NULL)
    {
        /* Deallocate contents of the private data, if any */
        /* Deallocate private data structure */
        g_free(priv);
        /* And finally set the opaque pointer back to NULL, so that
         *  we don't deallocate it twice. */
        (OL_OSD_TOOLBAR(toolbar))->priv = NULL;
    }
}

static void
ol_osd_toolbar_destroy (GtkObject *obj)
{
  ol_osd_toolbar_set_player (OL_OSD_TOOLBAR (obj), NULL);
}

GtkWidget *
ol_osd_toolbar_new (void)
{
  OlOsdToolbar *toolbar;
  toolbar = g_object_new (ol_osd_toolbar_get_type (), NULL);
  return GTK_WIDGET (toolbar);
}

void
ol_osd_toolbar_set_player (OlOsdToolbar *toolbar,
                           OlPlayer *player)
{
  ol_assert (OL_IS_OSD_TOOLBAR (toolbar));
  OlOsdToolbarPriv *priv = OL_OSD_TOOLBAR_GET_PRIVATE (toolbar);
  if (player == priv->player)
    return;
  if (player != NULL)
  {
    g_object_ref (player);
    g_signal_connect (player,
                      "status-changed",
                      G_CALLBACK (_status_changed_cb),
                      toolbar);
    g_signal_connect (player,
                      "caps-changed",
                      G_CALLBACK (_caps_changed_cb),
                      toolbar);
  }
  if (priv->player != NULL)
  {
    g_signal_handlers_disconnect_by_func (priv->player,
                                          _status_changed_cb,
                                          toolbar);
    g_signal_handlers_disconnect_by_func (priv->player,
                                          _caps_changed_cb,
                                          toolbar);
    g_object_unref (priv->player);
  }
  priv->player = player;
  _update_caps (toolbar);
  _update_status (toolbar);
}
