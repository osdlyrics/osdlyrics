/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011  Tiger Soldier <tigersoldi@gmail.com>
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

#include <gio/gio.h>
#include "ol_player.h"
#include "ol_consts.h"
#include "ol_timeline.h"
#include "ol_debug.h"

#define assert_variant_type(value, type)                        \
  do {                                                          \
    if (!g_variant_is_of_type ((value), G_VARIANT_TYPE (type))) \
    {                                                           \
      ol_errorf ("Except variant type of %s, but %s got\n",     \
                 type, g_variant_get_type_string ((value)));    \
    }                                                           \
  } while (0);

#define OL_PLAYER_GET_PRIVATE(object) \
  ((OlPlayerPrivate *)((OL_PLAYER(object))->priv))

const int POSITION_ACCURACY_MS = 1000;

enum {
  PLAYER_LOST,
  PLAYER_CONNECTED,
  TRACK_CHANGED,
  STATUS_CHANGED,
  CAPS_CHANGED,
  LAST_SIGNAL,
};

enum {
  CAPS_INDEX_PLAY = 0,
  CAPS_INDEX_PREV,
  CAPS_INDEX_NEXT,
  CAPS_INDEX_SEEK,
  CAPS_INDEX_STOP,
  CAPS_INDEX_PAUSE,
  LAST_CAPS_INDEX,
};

typedef struct _OlPlayerPrivate OlPlayerPrivate;

struct _OlPlayerPrivate
{
  GDBusProxy *proxy;
  GDBusProxy *mpris2_proxy;
  gboolean connected;
  OlMetadata *metadata;
  enum OlPlayerStatus status;
  enum OlPlayerCaps caps;
  gchar *player_name;
  gchar *player_icon;
  GCancellable *cancel_player_info;
  OlTimeline *timeline;
  gboolean initialized;
};

static guint signals[LAST_SIGNAL] = { 0 };
static struct
{
  const gchar *name;
  gint value;
} CAPS_NAME_MAP[] = {
  { "CanGoNext", CAPS_INDEX_NEXT },
  { "CanGoPrevious", CAPS_INDEX_PREV },
  { "CanPlay", CAPS_INDEX_PLAY },
  { "CanPause", CAPS_INDEX_PAUSE },
  { "CanSeek", CAPS_INDEX_SEEK },
};

static const gchar * const CAPS_INDEX_MAP[] = {
  "CanPlay", "CanGoPrevious", "CanGoNext", "CanSeek", "", "CanPause",
};

static void ol_player_class_init (OlPlayerClass *klass);
static void ol_player_init (OlPlayer *player);
static void ol_player_init_proxy (OlPlayer *player);
static void ol_player_init_mpris2_proxy (OlPlayer *player);
static void ol_player_finalize (GObject *object);
/* static void ol_player_set_property (GObject *object, */
/*                                     guint property_id, */
/*                                     const GValue *value, */
/*                                     GParamSpec *pspec); */
/* static void ol_player_get_property (GObject *object, */
/*                                     guint property_id, */
/*                                     GValue *value, */
/*                                     GParamSpec *pspec); */
static void ol_player_proxy_signal (GDBusProxy *proxy,
                                    gchar *sender_name,
                                    gchar *signal_name,
                                    GVariant *parameters,
                                    gpointer user_data);
static void ol_player_mpris2_proxy_signal (GDBusProxy *proxy,
                                           gchar *sender_name,
                                           gchar *signal_name,
                                           GVariant *parameters,
                                           gpointer user_data);
static void ol_player_mpris2_proxy_properties_changed (GDBusProxy *proxy,
                                                       GVariant   *changed_properties,
                                                       GStrv       invalidated_properties,
                                                       gpointer    user_data);
static void ol_player_fetch_player_info_async (OlPlayer *player);
static void ol_player_fetch_player_info_sync (OlPlayer *player);
static void ol_player_fetch_player_info_cb (GObject *source_object,
                                            GAsyncResult *res,
                                            gpointer user_data);
static void ol_player_set_player_info (OlPlayer *player,
                                       GVariant *value);
static void ol_player_update_position (OlPlayer *player,
                                       GVariant *value);
static void ol_player_update_metadata (OlPlayer *player,
                                       GVariant *value);
static void ol_player_update_status (OlPlayer *player,
                                     GVariant *value);
static void ol_player_update_caps (OlPlayer *player,
                                   gint changed_mask,
                                   GVariant **values);

static void ol_player_proxy_name_owner_changed (GObject *gobject,
                                                GParamSpec *pspec,
                                                gpointer user_data);
static void _cancel_call (GCancellable **cancellable);
static GVariant *ol_player_get_property (GDBusProxy *proxy,
                                         const char *name);

G_DEFINE_TYPE_WITH_PRIVATE (OlPlayer, ol_player, G_TYPE_OBJECT);

static void
ol_player_class_init (OlPlayerClass *klass)
{
  GObjectClass *gklass = G_OBJECT_CLASS (klass);

  /* gklass->set_property = ol_player_set_property; */
  /* gklass->get_property = ol_player_get_property; */
  gklass->finalize = ol_player_finalize;

  signals[PLAYER_LOST] =
    g_signal_new ("player-lost",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[PLAYER_CONNECTED] =
    g_signal_new ("player-connected",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[TRACK_CHANGED] =
    g_signal_new ("track-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[STATUS_CHANGED] =
    g_signal_new ("status-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
  signals[CAPS_CHANGED] =
    g_signal_new ("caps-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

OlPlayer *
ol_player_new (void)
{
  return OL_PLAYER (g_object_new (OL_TYPE_PLAYER, NULL));
}

static void
ol_player_init (OlPlayer *player)
{
  /* Allocate Private data structure */
  (OL_PLAYER(player))->priv = \
      (OlPlayerPrivate *) g_malloc0(sizeof(OlPlayerPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_PLAYER(player))->priv != NULL)
  {
    OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
    private->metadata = ol_metadata_new ();
    private->status = OL_PLAYER_UNKNOWN;
    private->caps = 0;
    private->initialized = FALSE;
    /* TODO: Initialize player info */
    private->timeline = ol_timeline_new ();
    ol_player_init_proxy (player);
    ol_player_init_mpris2_proxy (player);
    ol_player_fetch_player_info_async (player);
  }
}

static void
ol_player_dispose(GObject *object)
{
    OlPlayer *self = (OlPlayer *)object;
    OlPlayerPrivate *priv = OL_PLAYER_GET_PRIVATE(self);
    /* Check if not NULL! To avoid calling dispose multiple times */
    if(priv != NULL)
    {
        /* Deallocate contents of the private data, if any */
        /* Deallocate private data structure */
        g_free(priv);
        /* And finally set the opaque pointer back to NULL, so that
         *  we don't deallocate it twice. */
        (OL_PLAYER(self))->priv = NULL;
    }
}

static void
ol_player_init_proxy (OlPlayer *player)
{
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  GError *error = NULL;
  private->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                  G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                  NULL, /* GDBusInterfaceInfo */
                                                  OL_SERVICE_DAEMON,
                                                  OL_OBJECT_PLAYER,
                                                  OL_IFACE_PLAYER,
                                                  NULL, /* GCancellable */
                                                  &error);
  if (private->proxy == NULL && error != NULL) {
    ol_errorf ("Cannot connect to player object: %s", error->message);
    g_error_free (error);
    return;
  }
  g_signal_connect (private->proxy,
                    "g-signal",
                    G_CALLBACK (ol_player_proxy_signal),
                    player);
}

static void
ol_player_init_mpris2_proxy (OlPlayer *player)
{
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  GError *error = NULL;
  private->mpris2_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                                         NULL, /* GDBusInterfaceInfo */
                                                         OL_SERVICE_DAEMON,
                                                         OL_OBJECT_MPRIS2,
                                                         OL_IFACE_MPRIS2_PLAYER,
                                                         NULL, /* GCancellable */
                                                         &error);
  if (private->mpris2_proxy == NULL && error != NULL)
  {
    ol_errorf ("Cannot connect to MPRIS2 player object: %s", error->message);
    g_error_free (error);
    return;
  }
  g_signal_connect (private->mpris2_proxy,
                    "g-signal",
                    G_CALLBACK (ol_player_mpris2_proxy_signal),
                    player);
  g_signal_connect (private->mpris2_proxy,
                    "notify::g-name-owner",
                    G_CALLBACK (ol_player_proxy_name_owner_changed),
                    player);
  g_signal_connect (private->mpris2_proxy,
                    "g-properties-changed",
                    G_CALLBACK (ol_player_mpris2_proxy_properties_changed),
                    player);
}

static void
_cancel_call (GCancellable **cancellable)
{
  if (cancellable && *cancellable)
  {
    g_cancellable_cancel (*cancellable);
    g_object_unref (*cancellable);
    *cancellable = NULL;
  }
}

static void
ol_player_finalize (GObject *object)
{
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (object);
  if (private->proxy != NULL)
  {
    g_object_unref (private->proxy);
    private->proxy = NULL;
  }
  if (private->mpris2_proxy != NULL)
  {
    g_signal_handlers_disconnect_by_func (private->mpris2_proxy,
                                          "notify::g-name-owner",
                                          G_CALLBACK (ol_player_proxy_name_owner_changed));
    g_signal_handlers_disconnect_by_func (private->mpris2_proxy,
                                          "g-signal",
                                          G_CALLBACK (ol_player_mpris2_proxy_signal));
    g_object_unref (private->mpris2_proxy);
    private->mpris2_proxy = NULL;
  }
  if (private->metadata != NULL)
  {
    ol_metadata_free (private->metadata);
    private->metadata = NULL;
  }
  if (private->player_name != NULL)
  {
    g_free (private->player_name);
    private->player_name = NULL;
  }
  if (private->player_icon != NULL)
  {
    g_free (private->player_icon);
    private->player_icon = NULL;
  }
  _cancel_call (&private->cancel_player_info);
  ol_timeline_free (private->timeline);
  private->timeline = NULL;
  G_OBJECT_CLASS (ol_player_parent_class)->finalize (object);
}

static void
ol_player_proxy_signal (GDBusProxy *proxy,
                        gchar *sender_name,
                        gchar *signal_name,
                        GVariant *parameters,
                        gpointer user_data)
{
  ol_assert (OL_IS_PLAYER (user_data));
  OlPlayer *player = OL_PLAYER (user_data);
  if (g_strcmp0 (signal_name, "PlayerLost") == 0)
  {
    ol_player_set_player_info (player, NULL);
    g_signal_emit (player, signals[PLAYER_LOST], 0);
  }
  else if (g_strcmp0 (signal_name, "PlayerConnected") == 0)
  {
    assert_variant_type (parameters, "(a{sv})");
    GVariant *value;
    g_variant_get (parameters, "(@a{sv})", &value);
    ol_player_set_player_info (player, value);
    g_variant_unref (value);
    g_signal_emit (player, signals[PLAYER_CONNECTED], 0);
  }
}

static void
ol_player_mpris2_proxy_signal (GDBusProxy *proxy,
                               gchar *sender_name,
                               gchar *signal_name,
                               GVariant *parameters,
                               gpointer user_data)
{
  ol_assert (OL_IS_PLAYER (user_data));
  OlPlayer *player = OL_PLAYER (user_data);
  if (g_str_equal (signal_name, "Seeked"))
  {
    GVariant *value;
    g_variant_get (parameters, "(@x)", &value);
    ol_player_update_position (player, value);
    g_variant_unref (value);
  }
}

static void
ol_player_mpris2_proxy_properties_changed (GDBusProxy *proxy,
                                           GVariant *changed_properties,
                                           GStrv invalidated_properties,
                                           gpointer user_data)
{
  OlPlayer *player = OL_PLAYER (user_data);
  GVariantIter *iter = NULL;
  gchar *key;
  GVariant *value;
  gint caps_masks = 0;
  GVariant *caps_values[LAST_CAPS_INDEX] = {0};
  g_variant_get (changed_properties, "a{sv}", &iter);
  while (g_variant_iter_loop (iter, "{&sv}", &key, &value))
  {
    if (g_str_equal (key, "Metadata"))
    {
      ol_player_update_metadata (player, value);
    }
    else if (g_str_equal (key, "PlaybackStatus"))
    {
      ol_player_update_status (player, value);
    }
    else
    {
      gint i;
      for (i = 0; i < G_N_ELEMENTS (CAPS_NAME_MAP); i++)
      {
        if (g_str_equal (key, CAPS_NAME_MAP[i].name))
        {
          caps_masks |= (1 << CAPS_NAME_MAP[i].value);
          caps_values[CAPS_NAME_MAP[i].value] = g_variant_ref (value);
        }
      }
    }
  }
  if (caps_masks)
  {
    guint i;
    ol_player_update_caps (player, caps_masks, caps_values);
    for (i = 0; i < LAST_CAPS_INDEX; i++)
      if (caps_values[i])
        g_variant_unref (caps_values[i]);
  }
  g_variant_iter_free (iter);
}

static void
ol_player_proxy_name_owner_changed (GObject *gobject,
                                    GParamSpec *pspec,
                                    gpointer user_data)
{
  ol_assert (OL_IS_PLAYER (user_data));
  OlPlayer *player = OL_PLAYER (user_data);
  gchar *owner = NULL;
  if ((owner = g_dbus_proxy_get_name_owner (G_DBUS_PROXY (gobject))) == NULL)
  {
    /* Daemon disconnected, treat the player as lost */
    ol_debug ("Daemon lost");
    ol_player_set_player_info (player, NULL);
  }
  else
  {
    /* Daemon connected, get player info */
    ol_debugf ("Daemon connected: %s\n", owner);
    ol_player_fetch_player_info_async (player);
  }
}

static void
ol_player_fetch_player_info_sync (OlPlayer *player)
{
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  GError *error = NULL;
  GVariant *value = NULL;
  value = g_dbus_proxy_call_sync (private->proxy,
                                  "GetCurrentPlayer",
                                  NULL, /* parameters */
                                  G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                  -1,   /* timeout_msec */
                                  NULL, /* cancellable */
                                  &error);
  if (value == NULL)
  {
    ol_debugf ("Cannot get player info: %s\n", error->message);
    g_error_free (error);
  }
  else
  {
    assert_variant_type (value, "(ba{sv})");
    gboolean connected;
    GVariant *info = NULL;
    g_variant_get (value, "(b@a{sv})", &connected, &info);
    if (connected)
      ol_player_set_player_info (player, info);
    else
      ol_player_set_player_info (player, NULL);
    g_variant_unref (info);
    g_variant_unref (value);
  }
}

static void
ol_player_fetch_player_info_async (OlPlayer *player)
{
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (private->cancel_player_info)
    return;
  private->cancel_player_info = g_cancellable_new ();
  g_dbus_proxy_call (private->proxy,
                     "GetCurrentPlayer",
                     NULL,      /* parameters */
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     private->cancel_player_info,
                     ol_player_fetch_player_info_cb,
                     player);
}

static void
ol_player_fetch_player_info_cb (GObject *source_object,
                                GAsyncResult *res,
                                gpointer user_data)
{
  ol_assert (OL_IS_PLAYER (user_data));
  OlPlayer *player = OL_PLAYER (user_data);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  GError *error = NULL;
  GVariant *value =  g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object),
                                               res,
                                               &error);
  if (value)
  {
    assert_variant_type (value, "(ba{sv})");
    gboolean connected;
    GVariant *info = NULL;
    g_variant_get (value, "(b@a{sv})", &connected, &info);
    if (connected)
      ol_player_set_player_info (player, info);
    else
      ol_player_set_player_info (player, NULL);
    g_variant_unref (info);
    g_variant_unref (value);
  }
  else
  {
    ol_errorf ("Cannot get player info: %s\n", error->message);
    ol_player_set_player_info (player, NULL);
    g_error_free (error);
  }
  if (private->cancel_player_info)
  {
    g_object_unref (private->cancel_player_info);

  }
}

static void
ol_player_set_player_info (OlPlayer *player,
                           GVariant *value)
{
  ol_assert (OL_IS_PLAYER (player));
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  private->initialized = TRUE;
  g_free (private->player_name);
  private->player_name = NULL;
  g_free (private->player_icon);
  private->player_icon = NULL;
  if (value == NULL)
  {
    /* Player lost, clear the properties */
    if (private->connected)
    {
      ol_player_update_metadata (player, NULL);
      ol_player_update_position (player, NULL);
      ol_player_update_status (player, NULL);
      ol_player_update_caps (player,
                             OL_PLAYER_PLAY | OL_PLAYER_NEXT | OL_PLAYER_PREV |
                             OL_PLAYER_PAUSE | OL_PLAYER_SEEK,
                             NULL);
      private->connected = FALSE;
    }
  }
  else
  {
    assert_variant_type (value, "a{sv}");
    GVariantIter *iter = NULL;
    g_variant_get (value, "a{sv}", &iter);
    gchar *key;
    GVariant *dict_value;
    while (g_variant_iter_loop (iter, "{sv}", &key, &dict_value))
    {
      if (g_strcmp0 (key, "name") == 0)
      {
        const gchar *name = g_variant_get_string (dict_value, NULL);
        if (name)
        {
          private->player_name = g_strdup (name);
        }
        else
        {
          ol_errorf ("Cannot get the name of the player. Value type: %s",
                     g_variant_get_type_string (dict_value));
        }
      }
      else if (g_strcmp0 (key, "appname") == 0)
      {
        /* TODO: get appname */
      }
      else if (g_strcmp0 (key, "binname") == 0)
      {
        /* TODO: get binname */
      }
      else if (g_strcmp0 (key, "cmd") == 0)
      {
        /* TODO: get cmd */
      }
      else if (g_strcmp0 (key, "icon") == 0)
      {
        const gchar *icon = g_variant_get_string (dict_value, NULL);
        if (icon)
        {
          private->player_icon = g_strdup (icon);
        }
        else
        {
          ol_errorf ("Cannot get the icon of the player. Value type: %s",
                     g_variant_get_type_string (dict_value));
        }
      }
    }
    ol_debugf ("player name: %s, icon: %s\n",
               private->player_name,
               private->player_icon);
    g_variant_iter_free (iter);
    ol_player_update_metadata (player, NULL);
    ol_player_update_status (player, NULL);
    ol_player_update_caps (player,
                           OL_PLAYER_NEXT | OL_PLAYER_PREV | OL_PLAYER_PLAY |
                           OL_PLAYER_PAUSE | OL_PLAYER_SEEK, NULL);
    ol_player_update_position (player, NULL);
    private->connected = TRUE;
  }
}

static GVariant *
ol_player_get_property (GDBusProxy *proxy,
                        const char *name)
{
  GVariant *ret;
  ret = g_dbus_proxy_get_cached_property (proxy, name);
  if (!ret)
  {
    GError *error = NULL;
    GVariant *result;
    result = g_dbus_connection_call_sync (g_dbus_proxy_get_connection (proxy),
                                          g_dbus_proxy_get_name (proxy),
                                          g_dbus_proxy_get_object_path (proxy),
                                          "org.freedesktop.DBus.Properties",
                                          "Get",
                                          g_variant_new ("(ss)",
                                                         OL_IFACE_MPRIS2_PLAYER,
                                                         name),
                                          G_VARIANT_TYPE ("(v)"),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1, /* timeout */
                                          NULL,
                                          &error);
    if (!result)
    {
      ol_errorf ("Cannot get property %s: %s\n", name, error->message);
      g_error_free (error);
    }
    else
    {
      g_variant_get (result, "(v)", &ret);
      g_variant_unref (result);
    }
  }
  return ret;
}

static void
ol_player_update_position (OlPlayer *player,
                           GVariant *value)
{
  OlPlayerPrivate *priv = OL_PLAYER_GET_PRIVATE (player);
  if (!value)
    value = ol_player_get_property (priv->mpris2_proxy,
                                    "Position");
  else
    g_variant_ref (value);
  if (!value)
    return;
  ol_timeline_maybe_set_time (priv->timeline,
                              g_variant_get_int64 (value) / 1000);
  g_variant_unref (value);
}

static void
ol_player_update_metadata (OlPlayer *player,
                           GVariant *value)
{
  OlPlayerPrivate *priv = OL_PLAYER_GET_PRIVATE (player);
  if (!value)
    value = ol_player_get_property (priv->mpris2_proxy,
                                    "Metadata");
  else
    g_variant_ref (value);
  if (!value)
    return;
  if (priv->metadata)
    ol_metadata_free (priv->metadata);
  priv->metadata = ol_metadata_new_from_variant (value);
  g_signal_emit (player, signals[TRACK_CHANGED], 0);
  g_variant_unref (value);
  ol_timeline_set_time (priv->timeline, 0);
  ol_player_update_position (player, NULL);
}

static void
ol_player_update_status (OlPlayer *player,
                         GVariant *value)
{
  OlPlayerPrivate *priv = OL_PLAYER_GET_PRIVATE (player);
  if (!value)
    value = ol_player_get_property (priv->mpris2_proxy,
                                    "PlaybackStatus");
  else
    g_variant_ref (value);
  if (!value)
    return;
  const gchar *status_str = g_variant_get_string (value, NULL);
  if (g_str_equal (status_str, "Playing"))
  {
    priv->status = OL_PLAYER_PLAYING;
    ol_timeline_play (priv->timeline);
  }
  else if (g_str_equal (status_str, "Paused"))
  {
    priv->status = OL_PLAYER_PAUSED;
    ol_timeline_pause (priv->timeline);
  }
  else if (g_str_equal (status_str, "Stopped"))
  {
    priv->status = OL_PLAYER_STOPPED;
    ol_timeline_stop (priv->timeline);
  }
  else
  {
    ol_errorf ("Unknown playback status: %s\n", status_str);
    priv->status = OL_PLAYER_UNKNOWN;
  }
  g_signal_emit (player, signals[STATUS_CHANGED], 0);
}

static void
ol_player_update_caps (OlPlayer *player,
                       gint changed_mask,
                       GVariant **values)
{
  OlPlayerPrivate *priv = OL_PLAYER_GET_PRIVATE (player);
  guint i;
  GVariant *value;
  for (i = 0; i < LAST_CAPS_INDEX; i++)
  {
    guint cap_bit = 1 << i;
    if (changed_mask & cap_bit)
    {
      value = values ? values[i] : NULL;
      if (value)
        g_variant_ref (value);
      else
        value = ol_player_get_property (priv->mpris2_proxy,
                                        CAPS_INDEX_MAP[i]);
      if (!value)
        continue;
      if (g_variant_get_boolean (value))
        priv->caps |= cap_bit;
      else
        priv->caps &= ~cap_bit;
      g_variant_unref (value);
    }
  }
  if (priv->caps & OL_PLAYER_PLAY)
    priv->caps |= OL_PLAYER_STOP;
  g_signal_emit (player, signals[CAPS_CHANGED], 0);
}

gboolean
ol_player_is_connected (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->initialized)
  {
    ol_player_fetch_player_info_sync (player);
  }
  return private->connected;
}

const char*
ol_player_get_name (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), NULL);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->initialized)
  {
    ol_player_fetch_player_info_sync (player);
  }
  return private->player_name;
}

const char*
ol_player_get_icon_path (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), NULL);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->initialized)
  {
    ol_player_fetch_player_info_sync (player);
  }
  return private->player_icon;
}

gboolean
ol_player_get_metadata (OlPlayer *player, OlMetadata *metadata)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected)
    return FALSE;
  if (metadata != NULL)
    ol_metadata_copy (metadata, private->metadata);
  return TRUE;
}

gboolean
ol_player_get_position (OlPlayer *player, guint64 *pos_ms)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected)
    return FALSE;
  if (pos_ms)
  {
    *pos_ms = ol_timeline_get_time (private->timeline);
  }
  return TRUE;
}

enum OlPlayerStatus
ol_player_get_status (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), OL_PLAYER_ERROR);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  return private->status;
}

int
ol_player_get_caps (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), -1);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  return private->caps;
}

gboolean
ol_player_play (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected || !(private->caps & OL_PLAYER_PLAY))
    return FALSE;
  g_dbus_proxy_call (private->mpris2_proxy,
                     "Play",
                     NULL,      /* parameters */
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     NULL,      /* cancellable */
                     NULL,      /* callback */
                     NULL);     /* user_data */
  return TRUE;
}

gboolean
ol_player_pause (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected || !(private->caps & OL_PLAYER_PAUSE))
    return FALSE;
  g_dbus_proxy_call (private->mpris2_proxy,
                     "Pause",
                     NULL,      /* parameters */
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     NULL,      /* cancellable */
                     NULL,      /* callback */
                     NULL);     /* user_data */
  return TRUE;
}

gboolean
ol_player_stop (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected || !(private->caps & OL_PLAYER_STOP))
    return FALSE;
  g_dbus_proxy_call (private->mpris2_proxy,
                     "Stop",
                     NULL,      /* parameters */
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     NULL,      /* cancellable */
                     NULL,      /* callback */
                     NULL);     /* user_data */
  return TRUE;
}

gboolean
ol_player_prev (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected || !(private->caps & OL_PLAYER_PREV))
    return FALSE;
  g_dbus_proxy_call (private->mpris2_proxy,
                     "Previous",
                     NULL,      /* parameters */
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     NULL,      /* cancellable */
                     NULL,      /* callback */
                     NULL);     /* user_data */
  return TRUE;
}

gboolean
ol_player_next (OlPlayer *player)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected || !(private->caps & OL_PLAYER_NEXT))
    return FALSE;
  g_dbus_proxy_call (private->mpris2_proxy,
                     "Next",
                     NULL,      /* parameters */
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     NULL,      /* cancellable */
                     NULL,      /* callback */
                     NULL);     /* user_data */
  return TRUE;
}

gboolean
ol_player_seek (OlPlayer *player, guint64 pos_ms)
{
  ol_assert_ret (OL_IS_PLAYER (player), FALSE);
  OlPlayerPrivate *private = OL_PLAYER_GET_PRIVATE (player);
  if (!private->connected || !(private->caps & OL_PLAYER_SEEK))
    return FALSE;
  g_dbus_proxy_call (private->mpris2_proxy,
                     "SetPosition",
                     g_variant_new ("(x)", (gint64)pos_ms * 1000),
                     G_DBUS_CALL_FLAGS_NO_AUTO_START,
                     -1,        /* timeout_msec */
                     NULL,      /* cancellable */
                     NULL,      /* callback */
                     NULL);     /* user_data */
  return TRUE;
}
