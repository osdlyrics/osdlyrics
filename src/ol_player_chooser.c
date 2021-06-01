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
#include "ol_player_chooser.h"
#include "ol_app_chooser_widget.h"
#include "ol_app_info.h"
#include "ol_intl.h"
#include "ol_utils.h"
#include "ol_config_proxy.h"
#include "ol_debug.h"

#define OL_PLAYER_CHOOSER_GET_PRIVATE(obj) \
    ((OlPlayerChooserPrivate *)((OL_PLAYER_CHOOSER(obj))->priv))

static const guint SUPPORT_CHOOSER_INDEX = 0;
static const guint ALL_CHOOSER_INDEX = 1;
static const guint MAX_CHOOSER_HEIGHT = 96 * 3; /* about 3 lines of apps */
static const guint DEFAULT_APP_COLUMNS = 4;

G_DEFINE_TYPE (OlPlayerChooser, ol_player_chooser, GTK_TYPE_DIALOG)

typedef struct _OlPlayerChooserPage
{
  GtkToggleButton *page_button;
  GtkScrolledWindow *panel;
  OlAppChooserWidget *chooser;
  OlPlayerChooser *window;
} OlPlayerChooserPage;

typedef struct _OlPlayerChooserPrivate
{
  GPtrArray *pages;
  GtkBox *page_button_panel;
  GSList *page_button_group;
  GtkBox *chooser_panel;
  GtkBox *custom_cmd_panel;
  GtkWidget *remember_button;
  GtkEntry *cmd_entry;
  GtkWidget *launch_button;
  GtkLabel *info_label;
  GtkImage *info_icon;
  GAppInfo *launch_app;
} OlPlayerChooserPrivate;

static void _destroy (GtkObject *object);
static void _set_supported_players (OlPlayerChooser *chooser,
                                    GList *supported_players);
static void _app_activate_cb (OlAppChooserWidget *chooser,
                              GAppInfo *info,
                              OlPlayerChooser *window);
static void _launch_button_clicked_cb (GtkButton *button,
                                       OlPlayerChooser *window);
static void _launch_app (OlPlayerChooser *window,
                         GAppInfo *app_info);
static void _set_launch_app (OlPlayerChooser *window,
                             GAppInfo *app_info);
static void _remember_cmd_if_needed (OlPlayerChooser *window,
                                     const char *cmd);
static void _new_page (OlPlayerChooser *window,
                       const char *page_name);
static void _show_page (OlPlayerChooser *window,
                        OlPlayerChooserPage *page);
static void _show_page_by_index (OlPlayerChooser *window,
                                 guint index);
static void _page_button_toggled (GtkToggleButton *widget,
                                  OlPlayerChooserPage *page);
static OlAppChooserWidget *_get_chooser (OlPlayerChooser *window,
                                         guint index);
static void _setup_ui (OlPlayerChooser *window);
static void _set_sensitive (OlPlayerChooser *window,
                            gboolean sensitive);
static GtkEntryCompletion *_new_bin_completion (void);
static gboolean _prepend_cmd_to_list (const char *path,
                                      const char *filename,
                                      gpointer userdata);

static void
ol_player_chooser_class_init (OlPlayerChooserClass *klass)
{
  ol_log_func ();
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;

  gobject_class = G_OBJECT_CLASS (klass);
  object_class = (GtkObjectClass*)klass;
  ol_player_chooser_parent_class = g_type_class_peek_parent (klass);

  object_class->destroy = _destroy;
}

static void
ol_player_chooser_init (OlPlayerChooser *window)
{
  /* Allocate Private data structure */
  (OL_PLAYER_CHOOSER(window))->priv = \
    (OlPlayerChooserPrivate *) g_malloc0(sizeof(OlPlayerChooserPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_PLAYER_CHOOSER(window))->priv != NULL)
  {
    OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
    priv->pages = g_ptr_array_new_with_free_func (g_free);
    priv->page_button_group = NULL;
    _setup_ui (window);
  }
}

static void
ol_player_chooser_dispose(GObject *object)
{
    OlPlayerChooser *self = (OlPlayerChooser *)object;
    OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE(self);
    /* Check if not NULL! To avoid calling dispose multiple times */
    if(priv != NULL)
    {
        /* Deallocate contents of the private data, if any */
        /* Deallocate private data structure */
        g_free(priv);
        /* And finally set the opaque pointer back to NULL, so that
         *  we don't deallocate it twice. */
        (OL_PLAYER_CHOOSER(self))->priv = NULL;
    }
}

static void
_new_page (OlPlayerChooser *window,
           const char *page_name)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  OlPlayerChooserPage *page = g_new (OlPlayerChooserPage, 1);
  page->page_button = GTK_TOGGLE_BUTTON (gtk_radio_button_new_with_label (priv->page_button_group,
                                                                          page_name));

  priv->page_button_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (page->page_button));
  gtk_toggle_button_set_mode (page->page_button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (page->page_button), GTK_RELIEF_NONE);
  page->chooser = OL_APP_CHOOSER_WIDGET (ol_app_chooser_widget_new ());
  gtk_widget_show (GTK_WIDGET (page->chooser));
  page->panel = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new (NULL, NULL));
  gtk_scrolled_window_add_with_viewport (page->panel, GTK_WIDGET (page->chooser));
  GtkViewport *viewport = GTK_VIEWPORT (gtk_bin_get_child (GTK_BIN (page->panel)));
  gtk_viewport_set_shadow_type (viewport, GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (page->panel,
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  g_signal_connect (page->page_button,
                    "toggled",
                    G_CALLBACK (_page_button_toggled),
                    page);
  g_signal_connect (page->chooser,
                    "app-activate",
                    G_CALLBACK (_app_activate_cb),
                    window);
  _page_button_toggled (page->page_button, page);
  if (priv->pages->len > 0)
    gtk_box_pack_start (priv->page_button_panel,
                        gtk_vseparator_new (),
                        FALSE,
                        FALSE,
                        5);
  gtk_box_pack_start (priv->page_button_panel,
                      GTK_WIDGET (page->page_button),
                      FALSE,    /* expand */
                      FALSE,    /* fill */
                      0);       /* padding */
  gtk_box_pack_start (priv->chooser_panel,
                      GTK_WIDGET (page->panel),
                      TRUE,     /* expand */
                      TRUE,     /* fill */
                      0);       /* padding */
  g_ptr_array_add (priv->pages, page);
}

static void
_page_button_toggled (GtkToggleButton *widget,
                      OlPlayerChooserPage *page)
{
  gtk_widget_set_visible (GTK_WIDGET (page->panel),
                          gtk_toggle_button_get_active (widget));
}

static void
_show_page (OlPlayerChooser *window,
            OlPlayerChooserPage *page)
{
  ol_assert (page != NULL);
  gtk_toggle_button_set_active (page->page_button, TRUE);
}

static void
_show_page_by_index (OlPlayerChooser *window,
                     guint index)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  _show_page (window,
              g_ptr_array_index (priv->pages, index));
}

static OlAppChooserWidget *
_get_chooser (OlPlayerChooser *window,
              guint index)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  OlPlayerChooserPage *page = g_ptr_array_index (priv->pages,
                                                 index);
  ol_assert_ret (page != NULL, NULL);
  return page->chooser;
}

static void
_set_apps_to_page (OlPlayerChooser *window,
                   guint index,
                   GList *app_list)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  OlAppChooserWidget *chooser = _get_chooser (window, index);
  ol_app_chooser_widget_set_app_list (chooser, app_list, DEFAULT_APP_COLUMNS);
  GtkRequisition chooser_req;
  gint box_height;
  gtk_widget_size_request (GTK_WIDGET (chooser), &chooser_req);
  gtk_widget_get_size_request (GTK_WIDGET (priv->chooser_panel), NULL, &box_height);
  gint height = MIN (MAX_CHOOSER_HEIGHT,
                     MAX (chooser_req.height, box_height));
  if (height != box_height)
    gtk_widget_set_size_request (GTK_WIDGET (priv->chooser_panel),
                                 -1,
                                 height);
}

static void
_setup_ui (OlPlayerChooser *window)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  /* Setup info widgets */
  priv->info_label = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_line_wrap (priv->info_label, TRUE);
  gtk_misc_set_alignment (GTK_MISC (priv->info_label), 0.0, 0.0);
  priv->info_icon = GTK_IMAGE (gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO,
                                                         GTK_ICON_SIZE_DIALOG));
  GtkWidget *info_box = gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (info_box),
                      GTK_WIDGET (priv->info_icon),
                      FALSE,    /* expand */
                      FALSE,    /* fill */
                      0);
  gtk_box_pack_start (GTK_BOX (info_box),
                      GTK_WIDGET (priv->info_label),
                      TRUE,     /* expand */
                      TRUE,     /* fill */
                      0);
  /* Setup app choosers */
  priv->page_button_panel = GTK_BOX (gtk_hbox_new (FALSE, 0));
  priv->chooser_panel = GTK_BOX (gtk_hbox_new (FALSE, 0));
  _new_page (window, _("Supported players"));
  _new_page (window, _("All players"));
  _set_apps_to_page (window,
                     ALL_CHOOSER_INDEX,
                     g_app_info_get_all_for_type ("audio/mp3"));
  GtkWidget *apps_frame = gtk_frame_new (_("Choose a player to launch"));
  GtkBox *page_vbox = GTK_BOX (gtk_vbox_new (FALSE, 0));
  gtk_widget_show (GTK_WIDGET (page_vbox));
  gtk_box_pack_start (page_vbox,
                      GTK_WIDGET (priv->page_button_panel),
                      FALSE,    /* expand */
                      FALSE,    /* fill */
                      0);       /* padding */
  gtk_box_pack_end (page_vbox,
                    GTK_WIDGET (priv->chooser_panel),
                    FALSE,    /* expand */
                    FALSE,    /* fill */
                    0);       /* padding */
  gtk_container_add (GTK_CONTAINER (apps_frame), GTK_WIDGET (page_vbox));
  /* Setup custom command */
  priv->custom_cmd_panel = GTK_BOX (gtk_hbox_new (FALSE, 5));
  GtkWidget *cmd_label = gtk_label_new (_("Use command:"));
  GtkWidget *cmd_entry = gtk_entry_new ();
  priv->cmd_entry = GTK_ENTRY (cmd_entry);
  gtk_entry_set_activates_default (priv->cmd_entry, TRUE);
  gtk_entry_set_completion (priv->cmd_entry, _new_bin_completion ());
  GtkWidget *launch_button = gtk_button_new_with_label (_("Launch"));
  gtk_widget_set_can_default (launch_button, TRUE);
  gtk_window_set_default (GTK_WINDOW (window), launch_button);
  priv->launch_button = launch_button;
  g_signal_connect (launch_button,
                    "clicked",
                    G_CALLBACK (_launch_button_clicked_cb),
                    window);
  gtk_box_pack_start (priv->custom_cmd_panel, cmd_label, FALSE, TRUE, 0);
  gtk_box_pack_start (priv->custom_cmd_panel, cmd_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (priv->custom_cmd_panel, launch_button, FALSE, TRUE, 0);

  GtkWidget *final_hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *remember_button = gtk_check_button_new_with_label (_("Remember my choice"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (remember_button), TRUE);
  priv->remember_button = remember_button;
  gtk_box_pack_start (GTK_BOX (final_hbox), remember_button, FALSE, TRUE, 0);
  /* Setup the whole dialog */
  GtkWidget *vbox = gtk_dialog_get_content_area (GTK_DIALOG (window));
  gtk_box_set_spacing (GTK_BOX (vbox), 10);
  gtk_box_pack_start (GTK_BOX (vbox),
                      info_box,
                      FALSE,
                      FALSE,
                      0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      apps_frame,
                      FALSE,
                      FALSE,
                      0);
  gtk_box_pack_start (GTK_BOX (vbox),
                      GTK_WIDGET (priv->custom_cmd_panel),
                      FALSE,
                      TRUE,
                      0);
  gtk_box_pack_end (GTK_BOX (vbox),
                    final_hbox,
                    FALSE,
                    TRUE,
                    0);
  gtk_widget_show (vbox);
  gtk_widget_show_all (info_box);
  gtk_widget_show (apps_frame);
  gtk_widget_show_all (GTK_WIDGET (priv->page_button_panel));
  gtk_widget_show (GTK_WIDGET (priv->chooser_panel));
  gtk_widget_show_all (GTK_WIDGET (priv->custom_cmd_panel));
  gtk_widget_show_all (GTK_WIDGET (final_hbox));

  gtk_dialog_add_button (GTK_DIALOG (window),
                         GTK_STOCK_CLOSE,
                         GTK_RESPONSE_CLOSE);
  gtk_window_set_title (GTK_WINDOW (window), _("Choose a player to launch"));
  gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
}

static gboolean
_prepend_cmd_to_list (const char *path,
                      const char *filename,
                      gpointer userdata)
{
  GList **cmds_p = userdata;
  gchar *filepath = g_build_path (G_DIR_SEPARATOR_S, path, filename, NULL);
  if (g_file_test (filepath, G_FILE_TEST_IS_EXECUTABLE) &&
      !g_file_test (filepath, G_FILE_TEST_IS_DIR))
  {
    *cmds_p = g_list_prepend (*cmds_p, g_strdup (filename));
  }
  return TRUE;
}

static GtkEntryCompletion *
_new_bin_completion (void)
{
  const gchar *path_env = g_getenv ("PATH");
  gchar **pathdirs = g_strsplit (path_env, G_SEARCHPATH_SEPARATOR_S, 0);
  gchar **pathiter = pathdirs;
  GList *cmds_p[] = {NULL};
  for (; *pathiter != NULL; pathiter++)
  {
    gchar *path = *pathiter;
    ol_traverse_dir (path, FALSE, _prepend_cmd_to_list, cmds_p);
  }
  GList *cmds = cmds_p[0];
  cmds = g_list_sort (cmds, (GCompareFunc)strcasecmp);
  GtkListStore *list = gtk_list_store_new (1, G_TYPE_STRING);
  for (; cmds != NULL; cmds = cmds->next)
  {
    GtkTreeIter iter;
    gtk_list_store_append (list, &iter);
    gtk_list_store_set (list, &iter, 0, cmds->data, -1);
  }
  for (; cmds != NULL; cmds = g_list_delete_link (cmds, cmds))
    g_free (cmds->data);
  GtkEntryCompletion *comp = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (comp, GTK_TREE_MODEL (list));
  gtk_entry_completion_set_text_column (comp, 0);
  gtk_entry_completion_set_inline_completion (comp, TRUE);
  gtk_entry_completion_set_inline_selection (comp, TRUE);
  return comp;
}

static void
_destroy (GtkObject *object)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (object);
  if (priv->pages != NULL)
  {
    g_ptr_array_free (priv->pages, TRUE);
    priv->pages = NULL;
  }
  if (priv->launch_app)
  {
    g_object_unref (priv->launch_app);
    priv->launch_app = NULL;
  }
  GTK_OBJECT_CLASS (ol_player_chooser_parent_class)->destroy (object);
}

GtkWidget*
ol_player_chooser_new (GList *supported_players)
{
  ol_log_func ();
  GtkWidget *window = g_object_new (ol_player_chooser_get_type (), NULL);
  _set_supported_players (OL_PLAYER_CHOOSER (window), supported_players);
  return window;
}

static void
_set_supported_players (OlPlayerChooser *window,
                        GList *supported_players)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  _set_apps_to_page (window,
                     SUPPORT_CHOOSER_INDEX,
                     supported_players);
  if (supported_players != NULL)
  {
    _show_page_by_index (window, SUPPORT_CHOOSER_INDEX);
  }
  else
  {
    _show_page_by_index (window, ALL_CHOOSER_INDEX);
  }
}

static void
_app_activate_cb (OlAppChooserWidget *chooser,
                  GAppInfo *app_info,
                  OlPlayerChooser *window)
{
  ol_assert (G_IS_APP_INFO (app_info));
  _launch_app (window, app_info);
}

static void
_launch_app (OlPlayerChooser *window,
             GAppInfo *app_info)
{
  GError *err = NULL;
  if (!g_app_info_launch (app_info, NULL, NULL, &err))
  {
    ol_errorf ("Cannot launch %s: %s",
               g_app_info_get_commandline (app_info),
               err->message);
    gchar *title = g_strdup_printf (_("Failed to launch %s"),
                                    g_app_info_get_name (app_info));
    ol_player_chooser_set_info (window, title, err->message);
    ol_player_chooser_set_image_by_name (window, GTK_STOCK_DIALOG_ERROR);
    g_free (title);
    g_error_free (err);
  }
  else
  {
    _set_launch_app (window, app_info);
    gchar *title = g_strdup_printf (_("Launching %s"),
                                    g_app_info_get_name (app_info));
    gchar *desc = g_strdup_printf (_("OSD Lyrics is trying to launch and connect to %s. Please wait for a second."),
                                   g_app_info_get_name (app_info));
    ol_player_chooser_set_info (window, title, desc);
    g_free (title);
    g_free (desc);
    ol_player_chooser_set_image_by_gicon (window, g_app_info_get_icon (app_info));
    _set_sensitive (window, FALSE);
    if (OL_IS_PLAYER_CHOOSER (window))
    {
      _remember_cmd_if_needed (window, g_app_info_get_commandline (app_info));
      gtk_dialog_response (GTK_DIALOG (window), OL_PLAYER_CHOOSER_RESPONSE_LAUNCH);
    }
  }
}

static void
_set_launch_app (OlPlayerChooser *window,
                 GAppInfo *app_info)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  if (priv->launch_app)
  {
    g_object_unref (priv->launch_app);
  }
  if (app_info)
    g_object_ref (app_info);
  priv->launch_app = app_info;
}

static void
_launch_button_clicked_cb (GtkButton *button,
                           OlPlayerChooser *window)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  const char *cmd = gtk_entry_get_text (priv->cmd_entry);
  if (ol_is_string_empty (cmd))
    return;
  GAppInfo *app_info = G_APP_INFO (ol_app_info_new (cmd,
                                                    NULL,
                                                    NULL,
                                                    OL_APP_INFO_PREFER_DESKTOP_FILE,
                                                    NULL));
  _launch_app (window, app_info);
  g_object_unref (app_info);
}

static void
_remember_cmd_if_needed (OlPlayerChooser *window,
                         const char *cmd)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->remember_button)))
  {
    OlConfigProxy *config = ol_config_proxy_get_instance ();
    ol_config_proxy_set_string (config,
                                "General/startup-player",
                                cmd);
  }
}

void
ol_player_chooser_set_info (OlPlayerChooser *window,
                            const char *info,
                            const char *description)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  ol_assert (info != NULL);
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  gchar *text;
  if (description != NULL)
    text = g_strdup_printf ("%s\n\n%s", info, description);
  else
    text = g_strdup (info);
  gtk_label_set_text (priv->info_label, text);
  g_free (text);
}

void
ol_player_chooser_set_image_by_name (OlPlayerChooser *window,
                                     const char *icon_name)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  ol_assert (icon_name != NULL);
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  gtk_image_set_from_icon_name (priv->info_icon, icon_name, GTK_ICON_SIZE_DIALOG);
}

void
ol_player_chooser_set_image_by_gicon (OlPlayerChooser *window,
                                      GIcon *icon)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  ol_assert (G_IS_ICON (icon));
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  gtk_image_set_from_gicon (priv->info_icon, icon, GTK_ICON_SIZE_DIALOG);
}

void
ol_player_chooser_set_info_by_state (OlPlayerChooser *window,
                                     enum OlPlayerChooserState state)
{
  ol_assert (OL_IS_PLAYER_CHOOSER (window));
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  _set_sensitive (window, TRUE);
  switch (state)
  {
  case OL_PLAYER_CHOOSER_STATE_NO_PLAYER:
    ol_player_chooser_set_info (window,
                                _("There is no supported player running"),
                                _("Please choose a player below to launch"));
    ol_player_chooser_set_image_by_name (window, GTK_STOCK_DIALOG_INFO);
    break;
  case OL_PLAYER_CHOOSER_STATE_LAUNCH_FAIL: {
    gchar *title = g_strdup_printf (_("Failed to connect to %s"),
                                      g_app_info_get_name (priv->launch_app));
    gchar *desc = g_strdup_printf (_("%s is not supported by OSD Lyrics, or not running. Please launch another player"),
                                   g_app_info_get_name (priv->launch_app));
    ol_player_chooser_set_info (window, title, desc);
    ol_player_chooser_set_image_by_name (window, GTK_STOCK_DIALOG_ERROR);
    g_free (title);
    g_free (desc);
    _set_launch_app (window, NULL);
    break;
  }
  case OL_PLAYER_CHOOSER_STATE_CONNECTED:
    gtk_widget_hide (GTK_WIDGET (window));
    break;
  default:
    ol_errorf ("Unknown player chooser state %d\n", (int) state);
  }
}

static void
_set_sensitive (OlPlayerChooser *window,
                gboolean sensitive)
{
  OlPlayerChooserPrivate *priv = OL_PLAYER_CHOOSER_GET_PRIVATE (window);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->chooser_panel),
                            sensitive);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->page_button_panel),
                            sensitive);
  gtk_widget_set_sensitive (GTK_WIDGET (priv->custom_cmd_panel),
                            sensitive);
}
