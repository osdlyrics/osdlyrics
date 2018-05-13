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
#include <string.h>
#include "ol_scroll_module.h"
#include "ol_metadata.h"
#include "ol_scroll_window.h"
#include "ol_config_proxy.h"
#include "ol_color.h"
#include "ol_stock.h"
#include "ol_image_button.h"
#include "ol_menu.h"
#include "ol_app.h"
#include "ol_player.h"
#include "ol_lrc.h"
#include "config.h"

typedef struct _OlScrollModule OlScrollModule;

struct OlLrc;
const int MESSAGE_TIMEOUT_MS = 5000;

struct _OlScrollModule
{
  OlPlayer *player;
  OlMetadata *metadata;
  guint64 duration;
  OlLrc *lrc;
  OlScrollWindow *scroll;
  guint message_timer;
  GList *config_bindings;
};

typedef void (*_ConfigSetFunc) (OlConfigProxy *config,
                                const gchar *key,
                                OlScrollModule *module);

struct _ConfigBinding
{
  OlScrollModule *module;
  guint change_handler;
  _ConfigSetFunc setter;
};

struct _ConfigMapping
{
  const gchar *key;
  _ConfigSetFunc setter;
};

static OlScrollModule* ol_scroll_module_new (struct OlDisplayModule *module,
                                             OlPlayer *player);
static void ol_scroll_module_free (struct OlDisplayModule *module);
static void _metadata_changed_cb (OlPlayer *player,
                                  OlScrollModule *module);
static void _caps_changed_cb (OlPlayer *player,
                              OlScrollModule *module);
static void _update_metadata (OlScrollModule *module);
static void _update_caps (OlScrollModule *module);
static void ol_scroll_module_set_played_time (struct OlDisplayModule *module,
                                              guint64 played_time);
static void ol_scroll_module_set_lrc (struct OlDisplayModule *module,
                                      OlLrc *lrc);

static void ol_scroll_module_set_message (struct OlDisplayModule *module,
                                          const char *message);
static void ol_scroll_module_set_last_message (struct OlDisplayModule *module,
                                               const char *message);
static void ol_scroll_module_clear_message (struct OlDisplayModule *module);

static gboolean _window_configure_cb (GtkWidget *widget,
                                      GdkEventConfigure *event,
                                      gpointer userdata);
static void _set_metadata_as_text (OlScrollModule *module);
static GtkWidget* _toolbar_new (OlScrollModule *module);
static gboolean _close_clicked_cb (GtkButton *button,
                                   gpointer userdata);
static gboolean _button_release_cb (OlScrollWindow *scroll,
                                    GdkEventButton *event,
                                    gpointer userdata);
static void _seek_cb (OlScrollWindow *scroll,
                      guint id,
                      gdouble percentage,
                      gpointer userdata);
static void _scroll_cb (OlScrollWindow *osd,
                        GdkEventScroll *event,
                        gpointer data);
/* config handlers */
static struct _ConfigBinding *_bind_config (const char *key,
                                            _ConfigSetFunc setter,
                                            OlScrollModule *module);
static void _unbind_config (struct _ConfigBinding *binding);
static void _bind_all_config (OlScrollModule *osd);
static void _config_changed_cb (OlConfigProxy *config,
                                const char *key,
                                struct _ConfigBinding *binding);
/* config value handlers */
static void _font_changed_cb (OlConfigProxy *config,
                              const char *key,
                              OlScrollModule *module);
static void _size_changed_cb (OlConfigProxy *config,
                              const char *key,
                              OlScrollModule *module);
static void _pos_changed_cb (OlConfigProxy *config,
                             const char *key,
                             OlScrollModule *module);
static void _active_color_changed_cb (OlConfigProxy *config,
                                      const char *key,
                                      OlScrollModule *module);
static void _inactive_color_changed_cb (OlConfigProxy *config,
                                        const char *key,
                                        OlScrollModule *module);
static void _bg_color_changed_cb (OlConfigProxy *config,
                                  const char *key,
                                  OlScrollModule *module);
static void _opacity_changed_cb (OlConfigProxy *config,
                                 const char *key,
                                 OlScrollModule *module);
static void _scroll_mode_changed_cb (OlConfigProxy *config,
                                     const char *key,
                                     OlScrollModule *module);

static struct _ConfigMapping _config_mapping[] = {
  { "ScrollMode/width", _size_changed_cb },
  { "ScrollMode/height", _size_changed_cb },
  { "ScrollMode/x", _pos_changed_cb },
  { "ScrollMode/y", _pos_changed_cb },
  { "ScrollMode/font-name", _font_changed_cb },
  { "ScrollMode/active-lrc-color", _active_color_changed_cb },
  { "ScrollMode/inactive-lrc-color", _inactive_color_changed_cb },
  { "ScrollMode/bg-color", _bg_color_changed_cb },
  { "ScrollMode/opacity", _opacity_changed_cb },
  { "ScrollMode/scroll-mode", _scroll_mode_changed_cb },
};
static gboolean _config_is_setting = FALSE;

static gboolean
_button_release_cb (OlScrollWindow *scroll,
                    GdkEventButton *event,
                    gpointer data)
{
  if (event->button == 3)
  {
    gtk_menu_popup (GTK_MENU (ol_menu_get_popup ()),
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    event->button,
                    event->time);
    return TRUE;
  }
  return FALSE;
}

static void
_scroll_cb (OlScrollWindow *osd,
            GdkEventScroll *event,
            gpointer data)
{
  int doffset = 0;
  if (event->direction == GDK_SCROLL_DOWN ||
      event->direction == GDK_SCROLL_RIGHT)
    doffset = -200;
  else if (event->direction == GDK_SCROLL_UP ||
           event->direction == GDK_SCROLL_LEFT)
    doffset = 200;
  ol_app_adjust_lyric_offset (doffset);
}


static gboolean
_window_configure_cb (GtkWidget *widget,
                      GdkEventConfigure *event,
                      gpointer user_data)
{
  ol_assert_ret (GTK_IS_WINDOW (widget), FALSE);
  OlScrollModule *module = (OlScrollModule*) user_data;
  if (module == NULL)
    return FALSE;
  if (_config_is_setting)
    return FALSE;
  _config_is_setting = TRUE;
  gint width, height, x, y;
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
  gtk_window_get_position (GTK_WINDOW (widget), &x, &y);
  ol_config_proxy_set_int (config, "ScrollMode/width", width);
  ol_config_proxy_set_int (config, "ScrollMode/height", height);
  ol_config_proxy_set_int (config, "ScrollMode/x", x);
  ol_config_proxy_set_int (config, "ScrollMode/y", y);
  _config_is_setting = FALSE;
  return FALSE;
}

static void _seek_cb (OlScrollWindow *scroll,
                      guint id,
                      gdouble percentage,
                      gpointer userdata)
{
  OlScrollModule *module = userdata;
  if (module->lrc)
  {
    OlLrcIter *iter = ol_lrc_iter_from_id (module->lrc, id);
    if (!ol_lrc_iter_is_valid (iter))
    {
      ol_errorf ("Seek to an invalid ID: %u\n", id);
      ol_lrc_iter_free (iter);
      return;
    }
    gint64 iter_time = ol_lrc_iter_get_timestamp (iter);
    guint64 duration = ol_lrc_iter_get_duration (iter);
    ol_lrc_iter_free (iter);
    gint64 new_time = iter_time + duration * percentage;
    ol_player_seek (module->player, new_time);
    ol_scroll_window_set_progress (module->scroll,
                                   id,
                                   percentage);
    /* avoid players send played time before seeked. */
    g_usleep (200000);
  }
}

static void
_config_changed_cb (OlConfigProxy *config,
                    const char *key,
                    struct _ConfigBinding *binding)
{
  if (_config_is_setting)
    return;
  _config_is_setting = TRUE;
  binding->setter (config, key, binding->module);
  _config_is_setting = FALSE;
}

static void
_font_changed_cb (OlConfigProxy *config,
                  const char *key,
                  OlScrollModule *module)
{
  gchar *font = ol_config_proxy_get_string (config, "ScrollMode/font-name");
  ol_assert (font != NULL);
  ol_scroll_window_set_font_name (module->scroll, font);
  g_free (font);
}

static void
_size_changed_cb (OlConfigProxy *config,
                  const char *key,
                  OlScrollModule *module)
{
  gint width = ol_config_proxy_get_int (config, "ScrollMode/width");
  gint height = ol_config_proxy_get_int (config, "ScrollMode/height");
  gtk_window_resize (GTK_WINDOW (module->scroll), width, height);
}

static void
_pos_changed_cb (OlConfigProxy *config,
                 const char *key,
                 OlScrollModule *module)
{
  gint x = ol_config_proxy_get_int (config, "ScrollMode/x");
  gint y = ol_config_proxy_get_int (config, "ScrollMode/y");
  gtk_window_move (GTK_WINDOW (module->scroll), x, y);
}

static void
_active_color_changed_cb (OlConfigProxy *config,
                          const char *key,
                          OlScrollModule *module)
{
  char *color_str = ol_config_proxy_get_string (config, key);
  if (color_str != NULL)
  {
    OlColor color = ol_color_from_string (color_str);
    ol_scroll_window_set_active_color (module->scroll, color);
    g_free (color_str);
  }
}

static void
_inactive_color_changed_cb (OlConfigProxy *config,
                            const char *key,
                            OlScrollModule *module)
{
  char *color_str = ol_config_proxy_get_string (config, key);
  if (color_str != NULL)
  {
    OlColor color = ol_color_from_string (color_str);
    ol_scroll_window_set_inactive_color (module->scroll, color);
    g_free (color_str);
  }
}

static void
_bg_color_changed_cb (OlConfigProxy *config,
                      const char *key,
                      OlScrollModule *module)
{
  char *color_str = ol_config_proxy_get_string (config, key);
  if (color_str != NULL)
  {
    OlColor color = ol_color_from_string (color_str);
    ol_scroll_window_set_bg_color (module->scroll, color);
    g_free (color_str);
  }
}

static void
_opacity_changed_cb (OlConfigProxy *config,
                     const char *key,
                     OlScrollModule *module)
{

  double opacity = ol_config_proxy_get_double (config, key);
  ol_scroll_window_set_bg_opacity (module->scroll, opacity);
}

static void
_scroll_mode_changed_cb (OlConfigProxy *config,
                         const char *key,
                         OlScrollModule *module)
{
  char *scroll_mode = ol_config_proxy_get_string (config, key);
  if (scroll_mode != NULL)
  {
    enum OlScrollWindowScrollMode mode = OL_SCROLL_WINDOW_ALWAYS;
    if (g_ascii_strcasecmp (scroll_mode, "lines") == 0)
    {
      mode = OL_SCROLL_WINDOW_BY_LINES;
    }
    ol_scroll_window_set_scroll_mode (module->scroll, mode);
    g_free (scroll_mode);
  }
}

static gboolean
_close_clicked_cb (GtkButton *button,
                   gpointer userdata)
{
  gtk_main_quit ();
  return FALSE;
}

static GtkWidget *
_toolbar_new (OlScrollModule *module)
{
  GtkWidget *toolbar = gtk_hbox_new (FALSE, 0);
  OlImageButton *button = OL_IMAGE_BUTTON (ol_image_button_new ());
  GtkIconTheme *icontheme = gtk_icon_theme_get_default ();
  GtkIconInfo *info = gtk_icon_theme_lookup_icon (icontheme,
                                                  OL_STOCK_SCROLL_CLOSE,
                                                  16,
                                                  0);
  
  GdkPixbuf *image = gdk_pixbuf_new_from_file (gtk_icon_info_get_filename (info),
                                               NULL);
  ol_image_button_set_pixbuf (button, image);
  g_signal_connect (button, "clicked", G_CALLBACK (_close_clicked_cb), module);
  gtk_container_add (GTK_CONTAINER (toolbar),
                     GTK_WIDGET (button));
  gtk_icon_info_free (info);

  gtk_widget_show_all (toolbar);
  return toolbar;
}

static void
_bind_all_config (OlScrollModule *module)
{
  int i;
  for (i = 0; i < G_N_ELEMENTS (_config_mapping); i++)
  {
    struct _ConfigBinding *binding = _bind_config (_config_mapping[i].key,
                                                   _config_mapping[i].setter,
                                                   module);
    module->config_bindings = g_list_prepend (module->config_bindings, binding);
  }
}

static struct _ConfigBinding *
_bind_config (const char *key,
              _ConfigSetFunc setter,
              OlScrollModule *module)
{
  ol_assert_ret (key != NULL, FALSE);
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  struct _ConfigBinding *binding = g_new (struct _ConfigBinding, 1);
  gchar *signal = g_strdup_printf ("changed::%s", key);
  binding->module = module;
  binding->setter = setter;
  binding->change_handler = g_signal_connect (config,
                                              signal,
                                              (GCallback) _config_changed_cb,
                                              binding);
  g_free (signal);
  setter (config, key, module);
  return binding;
}

static void
_unbind_config (struct _ConfigBinding *binding)
{
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  ol_assert (binding != NULL);
  g_signal_handler_disconnect (config, binding->change_handler);
  g_free (binding);
}

static void
ol_scroll_module_init_scroll (OlScrollModule *module)
{
  ol_assert (module != NULL);
  module->scroll = OL_SCROLL_WINDOW (ol_scroll_window_new ());
  module->message_timer = 0;
  g_object_ref_sink(module->scroll);
  if (module->scroll == NULL)
  {
    return;
  }
  GtkWindow *window = GTK_WINDOW (module->scroll);
  gtk_window_set_title (window, PROGRAM_NAME);
  gtk_window_set_icon_name (window, PACKAGE_NAME);
  GtkWidget *toolbar = _toolbar_new (module);
  ol_scroll_window_add_toolbar (module->scroll,
                                toolbar);
  _bind_all_config (module);
  g_signal_connect (module->scroll, "configure-event",
                    G_CALLBACK (_window_configure_cb),
                    module);
  g_signal_connect (module->scroll, "button-release-event",
                    G_CALLBACK (_button_release_cb),
                    module);
  g_signal_connect (module->scroll, "scroll-event",
                    G_CALLBACK (_scroll_cb),
                    module);
  g_signal_connect (module->scroll, "seek",
                    G_CALLBACK (_seek_cb),
                    module);
  
  gtk_widget_show(GTK_WIDGET (module->scroll));
}

OlScrollModule*
ol_scroll_module_new (struct OlDisplayModule *module,
                      OlPlayer *player)
{
  OlScrollModule *priv = g_new (OlScrollModule, 1);
  g_object_ref (player);
  priv->player = player;
  priv->scroll = NULL;
  priv->lrc = NULL;
  priv->metadata = ol_metadata_new ();
  priv->config_bindings = NULL;
  ol_scroll_module_init_scroll (priv);
  g_signal_connect (player,
                    "track-changed",
                    G_CALLBACK (_metadata_changed_cb),
                    priv);
  _update_metadata (priv);
  g_signal_connect (player,
                    "caps-changed",
                    G_CALLBACK (_caps_changed_cb),
                    priv);
  _update_caps (priv);
  return priv;
}

void
ol_scroll_module_free (struct OlDisplayModule *module)
{
  ol_assert (module != NULL);
  OlScrollModule *priv = ol_display_module_get_data (module);
  ol_assert (priv != NULL);
  if (_config_is_setting)
    return;
  if (module == NULL)
    return;
  if (priv->scroll != NULL)
  {
    gtk_widget_destroy (GTK_WIDGET (priv->scroll));
    priv->scroll = NULL;
  }
  if (priv->message_timer > 0)
    g_source_remove (priv->message_timer);
  if (priv->player != NULL)
  {
    g_signal_handlers_disconnect_by_func (priv->player,
                                          _metadata_changed_cb,
                                          priv);
    g_signal_handlers_disconnect_by_func (priv->player,
                                          _caps_changed_cb,
                                          priv);
    g_object_unref (priv->player);
    priv->player = NULL;
  }
  if (priv->metadata != NULL)
  {
    ol_metadata_free (priv->metadata);
    priv->metadata = NULL;
  }
  if (priv->lrc)
  {
    g_object_unref (priv->lrc);
    priv->lrc = NULL;
  }
  while (priv->config_bindings != NULL)
  {
    _unbind_config (priv->config_bindings->data);
    priv->config_bindings = g_list_delete_link (priv->config_bindings,
                                                priv->config_bindings);
  }
  g_free (priv);
}

static void
_set_metadata_as_text (OlScrollModule *module)
{
  ol_assert (module != NULL);
  gchar *text = NULL;
  gchar *title = NULL;
  if (module->scroll != NULL)
  {
    if (ol_metadata_get_title (module->metadata) != NULL)
    {
      if (ol_metadata_get_artist (module->metadata) != NULL)
      {
        text = g_strdup_printf ("%s\n%s",
                                ol_metadata_get_title (module->metadata),
                                ol_metadata_get_artist (module->metadata));
        title = g_strdup_printf ("%s - %s - %s",
                                 ol_metadata_get_artist (module->metadata),
                                 ol_metadata_get_title (module->metadata),
                                 PROGRAM_NAME);
      }
      else
      {
        text = g_strdup_printf ("%s", ol_metadata_get_title (module->metadata));
        title = g_strdup_printf ("%s - %s",
                                 ol_metadata_get_title (module->metadata),
                                 PROGRAM_NAME);
      }
    }
    ol_scroll_window_set_text (module->scroll, text);
    if (!title)
    {
      gtk_window_set_title (GTK_WINDOW (module->scroll),
                            PROGRAM_NAME);
    }
    else
    {
      gtk_window_set_title (GTK_WINDOW (module->scroll),
                            title);
      g_free (title);
    }
    if (text != NULL)
      g_free (text);
  }
}

static void
_metadata_changed_cb (OlPlayer *player,
                                  OlScrollModule *module)
{
  _update_metadata (module);
}

static void
_update_metadata (OlScrollModule *module)
{
  ol_assert (module != NULL);
  ol_player_get_metadata (module->player, module->metadata);
  _set_metadata_as_text (module);
}

static void
_caps_changed_cb (OlPlayer *player,
                  OlScrollModule *module)
{
  _update_caps (module);
}

static void
_update_caps (OlScrollModule *module)
{
  ol_assert (module != NULL);
  ol_assert (module->player != NULL);
  ol_assert (module->scroll != NULL);
  ol_scroll_window_set_can_seek (module->scroll,
                                 ol_player_get_caps (module->player) & OL_PLAYER_SEEK);
}

static void
ol_scroll_module_set_played_time (struct OlDisplayModule *module,
                                  guint64 played_time)
{
  ol_assert (module != NULL);
  OlScrollModule *priv = ol_display_module_get_data (module);
  ol_assert (priv != NULL);
  if (priv->lrc != NULL && priv->scroll != NULL)
  {
    OlLrcIter *iter = ol_lrc_iter_from_timestamp (priv->lrc,
                                                  played_time);
    ol_scroll_window_set_progress (priv->scroll,
                                   ol_lrc_iter_get_id (iter),
                                   ol_lrc_iter_compute_percentage (iter, played_time));
    ol_lrc_iter_free (iter);
  }
}

void
ol_scroll_module_set_lrc (struct OlDisplayModule *module,
                          OlLrc *lrc)
{
  ol_log_func ();
  ol_assert (module != NULL);
  ol_assert (module != NULL);
  OlScrollModule *priv = ol_display_module_get_data (module);
  ol_assert (priv != NULL);
  if (priv->lrc != NULL)
    g_object_unref (priv->lrc);
  priv->lrc = lrc;
  if (priv->lrc == NULL)
    ol_scroll_window_set_whole_lyrics(priv->scroll, NULL);
  else
  {
    g_object_ref (priv->lrc);
    /*dump the whole lyrics of a song*/
    GPtrArray *text_array = g_ptr_array_new_with_free_func (g_free);
    OlLrcIter *iter = ol_lrc_iter_from_id (lrc, 0);
    const char *text = NULL;
    while (ol_lrc_iter_loop (iter, NULL, NULL, &text))
    {
      g_ptr_array_add (text_array, g_strdup (text));
    }
    ol_scroll_window_set_whole_lyrics(priv->scroll, text_array);
    g_ptr_array_unref (text_array);
    ol_lrc_iter_free (iter);
  }
}

static void
ol_scroll_module_set_message (struct OlDisplayModule *module,
                              const char *message)
{
  ol_assert (module != NULL);
  OlScrollModule *priv = ol_display_module_get_data (module);
  ol_assert (priv != NULL);
  if (priv->scroll != NULL && message != NULL)
  {
    ol_scroll_window_set_text (priv->scroll, message);
  }
}

static void
ol_scroll_module_set_last_message (struct OlDisplayModule *module,
                                   const char *message)
{
  ol_assert (module != NULL);
  OlScrollModule *priv = ol_display_module_get_data (module);
  ol_assert (priv != NULL);
  ol_scroll_module_set_message (module, message);
  if (priv->message_timer != 0)
    g_source_remove (priv->message_timer);
  priv->message_timer = g_timeout_add (MESSAGE_TIMEOUT_MS,
                                       (GSourceFunc) ol_scroll_module_clear_message,
                                       module);
}

static void
ol_scroll_module_clear_message (struct OlDisplayModule *module)
{
  ol_assert (module != NULL);
  OlScrollModule *priv = ol_display_module_get_data (module);
  ol_assert (priv != NULL);
  _set_metadata_as_text (priv);
  if (priv->message_timer > 0)
  {
    g_source_remove (priv->message_timer);
    priv->message_timer = 0;
  }
}

struct OlDisplayClass*
ol_scroll_module_get_class ()
{
  struct OlDisplayClass *klass = ol_display_class_new ("scroll",
                                                       (OlDisplayInitFunc) ol_scroll_module_new,
                                                       ol_scroll_module_free);
  klass->clear_message = ol_scroll_module_clear_message;
  klass->download_fail_message = ol_scroll_module_set_last_message;
  klass->search_fail_message = ol_scroll_module_set_last_message;
  klass->search_message = ol_scroll_module_set_message;
  klass->set_lrc = ol_scroll_module_set_lrc;
  /* klass->set_message = ol_scroll_module_set_message; */
  klass->set_played_time = ol_scroll_module_set_played_time;
  /* klass->set_player = ol_scroll_module_set_player; */
  /* klass->set_status = ol_scroll_module_set_status; */
  return klass;
}
