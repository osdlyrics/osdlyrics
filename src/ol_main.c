/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier
 *
 * This file is part of OSD Lyrics.
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
 * along with OSD Lyrics.  If not, see <http://www.gnu.org/licenses/>. 
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <gio/gio.h>
#include "config.h"
#include "ol_lrc.h"
#include "ol_player.h"
#include "ol_lyrics.h"
#include "ol_utils.h"
#include "ol_lyric_candidate_selector.h"
#include "ol_lyric_source.h"
#include "ol_trayicon.h"
#include "ol_intl.h"
#include "ol_config_updater.h"
#include "ol_config_proxy.h"
#include "ol_consts.h"
#include "ol_display_module.h"
#include "ol_keybindings.h"
#include "ol_stock.h"
#include "ol_app.h"
#include "ol_notify.h"
#include "ol_debug.h"
#include "ol_player_chooser.h"

#define REFRESH_INTERVAL 100
#define INFO_INTERVAL 500
#define TIMEOUT_WAIT_LAUNCH 5000

gboolean _arg_debug_cb (const gchar *option_name,
                        const gchar *value,
                        gpointer data,
                        GError **error);
static gboolean _arg_version;

static GOptionEntry cmdargs[] =
{
  { "debug", 'd', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, _arg_debug_cb,
    N_ ("The level of debug messages to log, can be 'none', 'error', 'debug', or 'info'"), "level" },
  { "version", 'v', 0, G_OPTION_ARG_NONE, &_arg_version,
    N_ ("Show version information"), NULL},
  { NULL }
};

static guint name_watch_id = 0;
static guint position_timer = 0;
static OlLyricSource *lyric_source = NULL;
static OlLyricSourceSearchTask *search_task = NULL;
static OlLyricSourceDownloadTask *download_task = NULL;
static OlPlayer *player = NULL;
static OlMetadata *current_metadata = NULL;
static OlLrc *current_lrc = NULL;
static OlLyrics *lyrics_proxy = NULL;
static char *display_mode = NULL;
static struct OlDisplayModule *display_module = NULL;
static gboolean initialized = FALSE;
static enum _PlayerLostAction {
  ACTION_NONE = 0,
  ACTION_LAUNCH_DEFAULT,
  ACTION_CHOOSE_PLAYER,
  ACTION_WAIT_LAUNCH,
  ACTION_QUIT,
} player_lost_action = ACTION_LAUNCH_DEFAULT;
static OlPlayerChooser *player_chooser = NULL;

static void _initialize (int argc, char **argv);
static void _wait_for_player_launch (void);
static void _player_chooser_response_cb (GtkDialog *dialog,
                                         gint response_id,
                                         gpointer user_data);
/* static void _search_callback (struct OlLrcFetchResult *result, */
/*                             void *userdata); */
/* static void _download_callback (struct OlLrcDownloadResult *result); */
static void _display_mode_changed (OlConfigProxy *config,
                                   const gchar *key,
                                   gpointer userdata);
static void _init_dbus_connection (void);
static void _init_dbus_connection_done (void);
static void _init_player (void);
static void _init_lyrics_proxy (void);
static void _client_name_acquired_cb (GDBusConnection *connection,
                                      const gchar *name,
                                      gpointer user_data);
static void _client_name_lost_cb (GDBusConnection *connection,
                                  const gchar *name,
                                  gpointer user_data);
static void _name_appeared_cb (GDBusConnection *connection,
                               const gchar *name,
                               const gchar *name_owner,
                               gpointer user_data);
static void _name_vanished_cb (GDBusConnection *connection,
                               const gchar *name,
                               gpointer user_data);
static void _ping_daemon (void);
static void _ping_daemon_cb (GDBusProxy *proxy,
                             GAsyncResult *res,
                             gpointer user_data);
static void _start_daemon_cb (GObject *source_object,
                              GAsyncResult *res,
                              gpointer user_data);
static void _track_changed_cb (void);
static void _status_changed_cb (void);
static void _player_lost_cb (void);
static void _player_connected_cb (void);
static void _start_position_timer (void);
static void _stop_position_timer (void);
static void _change_lrc (void);
static void _cancel_source_task (void);
static void _search_complete_cb (OlLyricSourceSearchTask *task,
                                 enum OlLyricSourceStatus status,
                                 GList *results,
                                 gpointer userdata);
static void _search_started_cb (OlLyricSourceSearchTask *task,
                                const gchar *sourceid,
                                const gchar *sourcename,
                                gpointer userdata);
static void _download_complete_cb (OlLyricSourceDownloadTask *task,
                                   enum OlLyricSourceStatus status,
                                   const gchar *content,
                                   guint len,
                                   gpointer userdata);

static void
_display_mode_changed (OlConfigProxy *config,
                       const gchar *key,
                       gpointer userdata)
{
  char *mode = ol_config_proxy_get_string (config, key);
  if (display_mode == NULL ||
      ol_stricmp (mode, display_mode, -1) != 0)
  {
    if (display_mode != NULL)
      g_free (display_mode);
    display_mode = g_strdup (mode);
    ol_display_module_free (display_module);
    display_module = ol_display_module_new (display_mode, player);
    ol_display_module_set_lrc (display_module, current_lrc);
  }
  g_free (mode);
}

static void
_download_complete_cb (OlLyricSourceDownloadTask *task,
                       enum OlLyricSourceStatus status,
                       const gchar *content,
                       guint len,
                       gpointer userdata)
{
  if (task == download_task)
  {
    if (status == OL_LYRIC_SOURCE_STATUS_SUCCESS)
    {
      GError *error = NULL;
      gchar *uri = ol_lyrics_set_content (lyrics_proxy,
                                          current_metadata,
                                          content,
                                          &error);
      if (!uri)
      {
        ol_errorf ("Set content failed: %s\n",
                   error->message);
        g_error_free (error);
      }
      else
      {
        ol_debugf ("Set content to %s\n", uri);
        g_free (uri);
      }
    }
    else if (status == OL_LYRIC_SOURCE_STATUS_FALIURE)
    {
      ol_display_module_download_fail_message (display_module, _("Fail to download lyric"));
    }
    download_task = NULL;
  }
  g_object_unref (task);
}

static void
_do_download (OlLyricSourceCandidate *candidate,
              const OlMetadata *metadata)
{
  if (!ol_metadata_equal (metadata, current_metadata))
    return;
  download_task = ol_lyric_source_download (lyric_source,
                                            candidate);
  g_object_ref (download_task);
  ol_display_module_set_message (display_module,
                                 _("Downloading lyric"),
                                 -1);
  g_signal_connect (G_OBJECT (download_task),
                    "complete",
                    G_CALLBACK (_download_complete_cb),
                    NULL);
}

static void
_search_complete_cb (OlLyricSourceSearchTask *task,
                     enum OlLyricSourceStatus status,
                     GList *results,
                     gpointer userdata)
{
  if (task == search_task)
  {
    if (status == OL_LYRIC_SOURCE_STATUS_SUCCESS && results != NULL)
    {
      if (display_module != NULL)
      {
        ol_display_module_clear_message (display_module);
      }
      /* TODO: show lyric select ui */
      ol_lyric_candidate_selector_show (results, current_metadata, _do_download);
    }
    else if ((status == OL_LYRIC_SOURCE_STATUS_SUCCESS && results == NULL) ||
             status == OL_LYRIC_SOURCE_STATUS_FALIURE)
    {
      if (display_module != NULL)
        ol_display_module_search_fail_message (display_module, _("Lyrics not found"));
    }
    search_task = NULL;
  }
  g_object_unref (task);
}

static void
_search_started_cb (OlLyricSourceSearchTask *task,
                    const gchar *sourceid,
                    const gchar *sourcename,
                    gpointer userdata)
{
  if (task == search_task && display_module != NULL)
  {
    char *msg = g_strdup_printf (_("Searching lyrics from %s"), sourcename);
    ol_display_module_search_message (display_module, msg);
    g_free (msg);
  }
}

gboolean
ol_app_download_lyric (OlMetadata *metadata)
{
  ol_log_func ();
  _cancel_source_task ();
  search_task = ol_lyric_source_search_default (lyric_source, metadata);
  g_object_ref (search_task);
  g_signal_connect (G_OBJECT (search_task),
                    "complete",
                    G_CALLBACK (_search_complete_cb),
                    NULL);
  g_signal_connect (G_OBJECT (search_task),
                    "started",
                    G_CALLBACK (_search_started_cb),
                    NULL);
  return TRUE;
}

OlLrc *
ol_app_get_current_lyric ()
{
  ol_log_func ();
  return current_lrc;
}

gboolean
ol_app_assign_lrcfile (OlMetadata *metadata,
                       const char *filepath,
                       gboolean update)
{
  ol_log_func ();
  ol_assert_ret (metadata != NULL, FALSE);
  ol_assert_ret (filepath == NULL || ol_path_is_file (filepath), FALSE);
  if (update)
  {
    char *uri = NULL;
    if (filepath == NULL)
      uri = g_strdup ("none:");
    else
      uri = g_filename_to_uri (filepath, NULL, NULL);
    GError *error = NULL;
    if (!ol_lyrics_assign (lyrics_proxy,
                           metadata,
                           uri,
                           &error))
    {
      ol_errorf ("Cannot assign lyric file: %s\n", error->message);
      g_error_free (error);
    }
  }
  return TRUE;
}

static void
_track_changed_cb (void)
{
  ol_log_func ();
  if (display_module != NULL)
    ol_display_module_set_lrc (display_module, NULL);
  ol_player_get_metadata (player, current_metadata);
  _change_lrc ();
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  if (ol_config_proxy_get_bool (config, "General/notify-music"))
    ol_notify_music_change (current_metadata, ol_player_get_icon_path (player));
}

static void
_cancel_source_task (void)
{
  if (search_task)
  {
    g_signal_handlers_disconnect_by_func (search_task, _search_complete_cb, NULL);
    g_signal_handlers_disconnect_by_func (search_task, _search_started_cb, NULL);
    ol_lyric_source_task_cancel (OL_LYRIC_SOURCE_TASK (search_task));
    g_object_unref (search_task);
    search_task = NULL;
  }
  if (download_task)
  {
    ol_lyric_source_task_cancel (OL_LYRIC_SOURCE_TASK (download_task));
    g_signal_handlers_disconnect_by_func (download_task, _download_complete_cb, NULL);
    g_object_unref (download_task);
    download_task = NULL;
  }
}

static void
_change_lrc (void)
{
  _cancel_source_task ();
  if (current_lrc)
    g_object_unref (current_lrc);
  current_lrc = ol_lyrics_get_current_lyrics (lyrics_proxy);
  if (display_module)
    ol_display_module_set_lrc (display_module, current_lrc);
  if (current_lrc == NULL &&
      !ol_is_string_empty (ol_metadata_get_title (current_metadata)))
    ol_app_download_lyric (current_metadata);
}

static void
_status_changed_cb (void)
{
  ol_log_func ();
  enum OlPlayerStatus status = ol_player_get_status (player);
  switch (status)
  {
  case OL_PLAYER_PLAYING:
  case OL_PLAYER_PAUSED:
    _start_position_timer ();
    break;
  case OL_PLAYER_STOPPED:
    _stop_position_timer ();
    break;
  default:
    /* In case of the daemon sent a wrong status but still playing, it's better
       to keep updating the position. */
    _start_position_timer ();
    break;
  }
  ol_trayicon_status_changed (ol_player_get_status (player));
}

static gboolean
_player_launch_timeout (gpointer userdata)
{
  _player_lost_cb ();
  return FALSE;
}

static void
_wait_for_player_launch (void)
{
  player_lost_action = ACTION_WAIT_LAUNCH;
  g_timeout_add (TIMEOUT_WAIT_LAUNCH, _player_launch_timeout, NULL);
}

static void
_player_chooser_response_cb (GtkDialog *dialog,
                             gint response_id,
                             gpointer user_data)
{
  ol_assert (GTK_IS_DIALOG (dialog));
  switch (response_id)
  {
  case OL_PLAYER_CHOOSER_RESPONSE_LAUNCH:
    _wait_for_player_launch ();
    break;
  case GTK_RESPONSE_DELETE_EVENT:
  case GTK_RESPONSE_CLOSE:
    gtk_widget_hide (GTK_WIDGET (dialog));
    if (!ol_player_is_connected (player))
      gtk_main_quit ();
    break;
  default:
    ol_errorf ("Unknown response id: %d\n", response_id);
  }
}

static void
_player_lost_cb (void)
{
  /* try to connect to other player first */
  if (ol_player_is_connected (player))
    return;
  _stop_position_timer ();
  switch (player_lost_action)
  {
  case ACTION_LAUNCH_DEFAULT:
  case ACTION_CHOOSE_PLAYER:
  {
    if (player_lost_action == ACTION_LAUNCH_DEFAULT)
    {
      OlConfigProxy *config = ol_config_proxy_get_instance ();
      char *player_cmd = ol_config_proxy_get_string (config,
                                                     "General/startup-player");
      if (!ol_is_string_empty (player_cmd))
      {
        ol_debugf ("Running %s\n", player_cmd);
        ol_launch_app (player_cmd);
        _wait_for_player_launch ();
        g_free (player_cmd);
        break;
      }
      else
      {
        g_free (player_cmd);
      }
    }
    if (!player_chooser)
    {
      /* TODO: */
      /* GList *supported_players = ol_player_get_support_players (); */
      GList *supported_players = NULL;
      player_chooser = OL_PLAYER_CHOOSER (ol_player_chooser_new (supported_players));
      g_signal_connect (player_chooser,
                        "response",
                        G_CALLBACK (_player_chooser_response_cb),
                        NULL);
      ol_player_chooser_set_info_by_state (player_chooser,
                                           OL_PLAYER_CHOOSER_STATE_NO_PLAYER);
    }
    else
    {
      ol_player_chooser_set_info_by_state (player_chooser,
                                           OL_PLAYER_CHOOSER_STATE_LAUNCH_FAIL);
    }
    gtk_widget_show (GTK_WIDGET (player_chooser));
    player_lost_action = ACTION_NONE;
    break;
  }
  case ACTION_QUIT:
    printf (_("No supported player is running, exit.\n"));
    gtk_main_quit ();
    break;
  default:
    break;
  }
}

static void
_player_connected_cb (void)
{
  if (player_chooser != NULL &&
      gtk_widget_get_visible (GTK_WIDGET (player_chooser)))
    ol_player_chooser_set_info_by_state (player_chooser,
                                         OL_PLAYER_CHOOSER_STATE_CONNECTED);
  if (!display_module)
  {
    /* Initialize display modules */
    OlConfigProxy *config = ol_config_proxy_get_instance ();
    ol_display_module_init ();
    display_mode = ol_config_proxy_get_string (config, "General/display-mode");
    display_module = ol_display_module_new (display_mode, player);
    g_signal_connect (config, "changed::General/display-mode",
                      G_CALLBACK (_display_mode_changed),
                      NULL);
  }
  player_lost_action = ACTION_QUIT;
  _start_position_timer ();
}

static gint
_update_position (gpointer data)
{
  ol_log_func ();
  guint64 time = 0;
  ol_player_get_position (player, &time);
  if (display_module)
    ol_display_module_set_played_time (display_module, time);
  return TRUE;
}

OlPlayer*
ol_app_get_player (void)
{
  return player;
}

OlLyricSource *
ol_app_get_lyric_source (void)
{
  return lyric_source;
}

OlLyrics *
ol_app_get_lyrics_proxy (void)
{
  return lyrics_proxy;
}

OlMetadata*
ol_app_get_current_music (void)
{
  return current_metadata;
}

void
ol_app_adjust_lyric_offset (int offset_ms)
{
  ol_log_func ();
  OlLrc *lrc = ol_app_get_current_lyric ();
  if (lrc == NULL)
    return;
  int old_offset = ol_lrc_get_offset (lrc);
  int new_offset = old_offset - offset_ms;
  ol_lrc_set_offset (lrc, new_offset);
}

static void
_parse_cmd_args (int *argc, char ***argv)
{
  ol_log_func ();
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new ("- Display your lyrics");
  g_option_context_add_main_entries (context, cmdargs, PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (!g_option_context_parse (context, argc, argv, &error))
  {
    ol_errorf ("option parsing failed: %s\n", error->message);
  }
  if (_arg_version)
  {
    printf ("%s %s\n", PROGRAM_NAME, VERSION);
    exit (0);
  }
  g_option_context_free (context);
}

gboolean
_arg_debug_cb (const gchar *option_name,
               const gchar *value,
               gpointer data,
               GError **error)
{
  if (value == NULL)
    value = "debug";
  if (strcmp (value, "none") == 0)
  {
    ol_log_set_level (OL_LOG_NONE);
  }
  else if (strcmp (value, "error") == 0)
  {
    ol_log_set_level (OL_ERROR);
  }
  else if (strcmp (value, "debug") == 0)
  {
    ol_log_set_level (OL_DEBUG);
  }
  else if (strcmp (value, "info") == 0)
  {
    ol_log_set_level (OL_INFO);
  }
  else
  {
    g_set_error_literal (error, g_quark_from_string (PACKAGE_NAME), 1,
                         N_ ("debug level should be one of ``none'', ``error'', ``debug'', or ``info''"));
    return FALSE;
  }
  return TRUE;
}

static void
_initialize (int argc, char **argv)
{
  ol_log_func ();
#if ENABLE_NLS
  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif

#ifndef GLIB_VERSION_2_32
  g_thread_init(NULL);
#endif
  gtk_init (&argc, &argv);
  _parse_cmd_args (&argc, &argv);
  g_set_prgname (_(PROGRAM_NAME));
  initialized = FALSE;
  g_bus_own_name (G_BUS_TYPE_SESSION,
                  OL_CLIENT_BUS_NAME,
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  NULL,         /* bus_acquired_handler */
                  _client_name_acquired_cb,
                  _client_name_lost_cb,
                  NULL,         /* user_data */
                  NULL);        /* user_data_free_func */
}

static void
_client_name_acquired_cb (GDBusConnection *connection,
                          const gchar *name,
                          gpointer user_data)
{
  ol_debugf ("Client bus name acquired\n");
  _init_dbus_connection ();
}

static void
_client_name_lost_cb (GDBusConnection *connection,
                      const gchar *name,
                      gpointer user_data)
{
  ol_debugf ("Client bus name lost\n");
  printf ("%s\n", _("Another OSD Lyrics is running, exit."));
  exit (1);
}

static void
_name_appeared_cb (GDBusConnection *connection,
                   const gchar *name,
                   const gchar *name_owner,
                   gpointer user_data)
{
  ol_debug ("Daemon appeared");
  _ping_daemon ();
  if (!initialized)
    _init_dbus_connection_done ();
}

static void
_ping_daemon (void)
{
  ol_debugf ("Starting to ping the daemon\n");
  GError *error = NULL;
  GDBusProxy *daemon_proxy =
    g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                   G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                                   NULL, /* interface_info */
                                   OL_SERVICE_DAEMON,
                                   OL_OBJECT_DAEMON,
                                   OL_IFACE_DAEMON,
                                   NULL, /* cancellable */
                                   &error);
  if (daemon_proxy != NULL)
  {
    g_dbus_proxy_call (daemon_proxy,
                       "Hello",
                       g_variant_new ("(s)", OL_CLIENT_BUS_NAME),
                       G_DBUS_CALL_FLAGS_NO_AUTO_START,
                       -1,
                       NULL,    /* cancellable */
                       (GAsyncReadyCallback)_ping_daemon_cb,
                       NULL);   /* user_data */
  }
  else
  {
    ol_errorf ("Cannot create daemon proxy: %s\n", error->message);
    g_error_free (error);
  }
}

static void
_ping_daemon_cb (GDBusProxy *proxy,
                 GAsyncResult *res,
                 gpointer user_data)
{
  GError *error = NULL;
  GVariant *ret = g_dbus_proxy_call_finish (proxy,
                                            res,
                                            &error);
  if (ret)
  {
    ol_debugf ("Succeed to ping the daemon\n");
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Fail to ping the daemon: %s\n", error->message);
    g_error_free (error);
  }
  g_object_unref (proxy);
}

static void
_name_vanished_cb (GDBusConnection *connection,
                   const gchar *name,
                   gpointer user_data)
{
  ol_debugf ("Daemon lost, try to activate it\n");
  g_dbus_connection_call (connection,
                          "org.freedesktop.DBus",  /* bus name */
                          "/org/freedesktop/DBus", /* object path */
                          "org.freedesktop.DBus",  /* interface name */
                          "StartServiceByName",    /* method name */
                          g_variant_new ("(su)", OL_SERVICE_DAEMON, 0),
                          G_VARIANT_TYPE ("(u)"),
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          (GAsyncReadyCallback) _start_daemon_cb,
                          NULL);
  ol_debugf ("Daemon appeared\n");
}

static void
_start_daemon_cb (GObject *source_object,
                  GAsyncResult *res,
                  gpointer user_data)
{
  GVariant *result;
  GError *error = NULL;
  result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source_object),
                                          res,
                                          &error);
  if (result)
  {
    guint32 start_service_result;
    g_variant_get (result, "(u)", &start_service_result);

    if (start_service_result != 1 && /* DBUS_START_REPLY_SUCCESS */
        start_service_result != 2) /* DBUS_START_REPLY_ALREADY_RUNNING */
    {
      ol_errorf ("Unexpected reply %u from StartServiceByName() method\n",
                 (guint)start_service_result);
    }
    /* We should do nothing, _name_appeared_cb will be called by name watch */
  }
  else
  {
    ol_errorf ("Unable to start daemon: %s\n", error->message);
    g_error_free (error);
    /* TODO: notify the user that there is a fatal error */
  }
}

static void
_init_dbus_connection (void)
{
  /* Activate the daemon */
  name_watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                    OL_SERVICE_DAEMON,
                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                                    _name_appeared_cb,
                                    _name_vanished_cb,
                                    NULL,  /* user_data */
                                    NULL); /* user_data_free_func */
}

static void
_init_player (void)
{
  player = ol_player_new ();
  g_object_ref_sink (player);
  g_signal_connect (player,
                    "track-changed",
                    _track_changed_cb,
                    NULL);
  g_signal_connect (player,
                    "status-changed",
                    _status_changed_cb,
                    NULL);
  g_signal_connect (player,
                    "player-lost",
                    _player_lost_cb,
                    NULL);
  g_signal_connect (player,
                    "player-connected",
                    _player_connected_cb,
                    NULL);
}

static void
_init_lyrics_proxy (void)
{
  lyrics_proxy = ol_lyrics_new (NULL);
  g_object_ref_sink (lyrics_proxy);
  g_signal_connect (lyrics_proxy,
                    "lyrics-changed",
                    G_CALLBACK (_change_lrc),
                    NULL);
}

static void
_init_lyric_source (void)
{
  lyric_source = ol_lyric_source_new ();
  g_object_ref_sink (lyric_source);
}

static void
_init_dbus_connection_done (void)
{
  ol_config_update ();
  current_metadata = ol_metadata_new ();
  _init_player ();
  _init_lyrics_proxy ();
  _init_lyric_source ();
  ol_stock_init ();
  ol_trayicon_init ();
  ol_notify_init ();
  ol_keybinding_init ();

  if (ol_player_is_connected (player))
  {
    _player_connected_cb ();
  }
  else
  {
    _player_lost_cb ();
  }
  _track_changed_cb ();
  _status_changed_cb ();
  initialized = TRUE;
}

static void
_uninitialize (void)
{
  _stop_position_timer ();
  g_signal_handlers_disconnect_by_func (player, _status_changed_cb, NULL);
  g_signal_handlers_disconnect_by_func (player, _track_changed_cb, NULL);
  g_object_unref (player);
  g_object_unref (lyrics_proxy);
  player = NULL;

  _cancel_source_task ();
  g_object_unref (lyric_source);
  lyric_source = NULL;

  g_bus_unwatch_name (name_watch_id);
  ol_metadata_free (current_metadata);
  current_metadata = NULL;
  ol_notify_unload ();
  if (display_module != NULL)
  {
    ol_display_module_free (display_module);
    display_module = NULL;
  }
  if (display_mode != NULL)
  {
    g_free (display_mode);
    display_mode = NULL;
  }
  if (player_chooser != NULL)
  {
    gtk_widget_destroy (GTK_WIDGET (player_chooser));
    player_chooser = NULL;
  }
  ol_display_module_unload ();
  ol_trayicon_free ();
  ol_config_proxy_unload ();
}

static void
_start_position_timer (void)
{
  if (!position_timer)
    position_timer = g_timeout_add (REFRESH_INTERVAL,
                                    _update_position,
                                    NULL);
}

static void
_stop_position_timer (void)
{
  if (position_timer)
  {
    g_source_remove (position_timer);
    position_timer = 0;
  }
}

int
main (int argc, char **argv)
{
  _initialize (argc, argv);
  gtk_main ();
  _uninitialize ();
  return 0;
}
