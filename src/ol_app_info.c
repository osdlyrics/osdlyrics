
/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011 Tiger Soldier <tigersoldier@gmail.com>
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
#include "ol_app_info.h"
#include "ol_intl.h"
#include "ol_debug.h"

struct _OlAppInfo
{
  GObject parent;
  gchar *cmdline;
  gchar *name;
  GIcon *icon;
  gchar *binfile;
  gboolean should_show;
};

static gchar *_str_join_argv (gchar **argv);
static gboolean _shell_arg_need_quote (const gchar *arg);
static gchar *_find_binfile (const gchar *binfile, gboolean match_prefix);
static gchar *_find_file_in_path_list (GList *path_list,
                                       const gchar *prefix,
                                       const gchar *suffix,
                                       gboolean match_prefix,
                                       gboolean (*file_test_func) (const gchar *));
static gboolean _file_is_executable (const gchar *filename);
static gboolean _file_exists (const gchar *filename);
static GIcon *_icon_new_from_name (const gchar *icon_name);
static void _app_info_set_from_desktop_file (OlAppInfo *info,
                                             enum OlAppInfoFlags flags);
static void ol_app_info_finalize (GObject *object);
static void ol_app_info_iface_init (GAppInfoIface *iface);
static void ol_app_info_class_init (OlAppInfoClass *klass);
static void ol_app_info_init (OlAppInfo *info);
static void _strv_replace (gchar **argv,
                           guint index,
                           const gchar *new_value);
static GList *_get_desktop_file_path_list ();
static gpointer _get_desktop_file_path_list_once (gpointer data);
static GList *_prepend_subdirs (GList *list);
/* -----------GAppInfo interfaces--------------- */
static GAppInfo *_app_info_dup (GAppInfo *appinfo);
static gboolean _app_info_equal (GAppInfo *appinfo1,
                                 GAppInfo *appinfo2);
static const char *_app_info_get_id (GAppInfo *appinfo);
static const char *_app_info_get_name (GAppInfo *appinfo);
static const char *_app_info_get_display_name (GAppInfo *appinfo);
static const char *_app_info_get_description (GAppInfo *appinfo);
static const char *_app_info_get_executable (GAppInfo *appinfo);
static const char *_app_info_get_commandline (GAppInfo *appinfo);
static GIcon *_app_info_get_icon (GAppInfo *appinfo);
static gboolean _app_info_launch (GAppInfo *appinfo,
                                  GList *files,
                                  GAppLaunchContext *launch_context,
                                  GError **error);
static gboolean _app_info_supports_uris (GAppInfo *appinfo);
static gboolean _app_info_supports_files (GAppInfo *appinfo);
static gboolean _app_info_launch_uris (GAppInfo *appinfo,
                                       GList *uris,
                                       GAppLaunchContext *launch_context,
                                       GError **error);
static gboolean _app_info_should_show (GAppInfo *appinfo);
static gboolean _app_info_set_as_default_for_type (GAppInfo *appinfo,
                                                   const char *content_type,
                                                   GError **error);
static gboolean _app_info_set_as_default_for_extension (GAppInfo *appinfo,
                                                        const char *extension,
                                                        GError **error);
static gboolean _app_info_add_supports_type (GAppInfo *appinfo,
                                             const char *content_type,
                                             GError **error);
static gboolean _app_info_can_remove_supports_type (GAppInfo *appinfo);
static gboolean _app_info_remove_supports_type (GAppInfo *appinfo,
                                                const char *content_type,
                                                GError **error);
static gboolean _app_info_can_delete (GAppInfo *appinfo);
static gboolean _app_info_do_delete (GAppInfo *appinfo);

G_DEFINE_TYPE_WITH_CODE (OlAppInfo, ol_app_info, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_APP_INFO,
                                                ol_app_info_iface_init));

static void
ol_app_info_class_init (OlAppInfoClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ol_app_info_finalize;
}

static void
ol_app_info_init (OlAppInfo *info)
{
}

static void
ol_app_info_iface_init (GAppInfoIface *iface)
{
  iface->dup = _app_info_dup;
  iface->equal = _app_info_equal;
  iface->get_id = _app_info_get_id;
  iface->get_name = _app_info_get_name;
  iface->get_description = _app_info_get_description;
  iface->get_executable = _app_info_get_executable;
  iface->get_icon = _app_info_get_icon;
  iface->launch = _app_info_launch;
  iface->supports_uris = _app_info_supports_uris;
  iface->supports_files = _app_info_supports_files;
  iface->launch_uris = _app_info_launch_uris;
  iface->should_show = _app_info_should_show;
  iface->set_as_default_for_type = _app_info_set_as_default_for_type;
  iface->set_as_default_for_extension = _app_info_set_as_default_for_extension;
  iface->add_supports_type = _app_info_add_supports_type;
  iface->can_remove_supports_type = _app_info_can_remove_supports_type;
  iface->remove_supports_type = _app_info_remove_supports_type;
  iface->can_delete = _app_info_can_delete;
  iface->do_delete = _app_info_do_delete;
  iface->get_commandline = _app_info_get_commandline;
  iface->get_display_name = _app_info_get_display_name;
}

static GAppInfo *
_app_info_dup (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);
  OlAppInfo *new_info;

  new_info = g_object_new (OL_TYPE_APP_INFO, NULL);
  new_info->cmdline = g_strdup (info->cmdline);
  new_info->name = g_strdup (info->name);
  new_info->binfile = g_strdup (info->binfile);
  if (info->icon)
    new_info->icon = g_object_ref (info->icon);
  new_info->should_show = info->should_show;
  return G_APP_INFO (new_info);
}

static gboolean
_app_info_equal (GAppInfo *appinfo1,
                 GAppInfo *appinfo2)
{
  OlAppInfo *info1 = OL_APP_INFO (appinfo1);
  OlAppInfo *info2 = OL_APP_INFO (appinfo2);

  return strcmp (info1->cmdline, info2->cmdline) == 0;
}

static const char *
_app_info_get_id (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);

  return info->cmdline;
}

static const char *
_app_info_get_name (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);

  if (info->name == NULL)
    return _("Unnamed");
  return info->name;
}

static const char *
_app_info_get_display_name (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);

  if (info->name == NULL)
    return _("Unnamed");
  return info->name;
}

static const char *
_app_info_get_description (GAppInfo *appinfo)
{
  return "";
}

static const char *
_app_info_get_executable (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);

  return info->binfile;
}

static const char *
_app_info_get_commandline (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);

  return info->cmdline;
}

static GIcon *
_app_info_get_icon (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);

  return info->icon;
}

static gboolean
_app_info_launch (GAppInfo *appinfo,
                  GList *files,
                  GAppLaunchContext *launch_context,
                  GError **error)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);
  /* TODO: implement launching. */
  GAppInfo* app = g_app_info_create_from_commandline (info->cmdline, "", 0, NULL);
  gboolean ret = g_app_info_launch (app, files, launch_context, error);
  g_object_unref (G_OBJECT (app));
  return ret;
}

static gboolean
_app_info_supports_uris (GAppInfo *appinfo)
{
  return FALSE;
}

static gboolean
_app_info_supports_files (GAppInfo *appinfo)
{
  return FALSE;
}

static gboolean
_app_info_launch_uris (GAppInfo *appinfo,
                       GList *uris,
                       GAppLaunchContext *launch_context,
                       GError **error)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);
  /* TODO: implement launching. */
  GAppInfo* app = g_app_info_create_from_commandline (info->cmdline, "", 0, NULL);
  gboolean ret = g_app_info_launch_uris (app, uris, launch_context, error);
  g_object_unref (G_OBJECT (app));
  return ret;
}

static gboolean
_app_info_should_show (GAppInfo *appinfo)
{
  OlAppInfo *info = OL_APP_INFO (appinfo);
  return info->should_show;
}

static gboolean
_app_info_set_as_default_for_type (GAppInfo *appinfo,
                                   const char *content_type,
                                   GError **error)
{
  if (error)
    *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                          0,    /* error code */
                          "OlAppInfo does not support set_as_default_for_type");
  return FALSE;
}

static gboolean
_app_info_set_as_default_for_extension (GAppInfo *appinfo,
                                        const char *extension,
                                        GError **error)
{
  if (error)
    *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                          0,    /* error code */
                          "OlAppInfo does not support set_as_default_for_extension");
  return FALSE;
}

static gboolean
_app_info_add_supports_type (GAppInfo *appinfo,
                             const char *content_type,
                             GError **error)
{
  if (error)
    *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                          0,    /* error code */
                          "OlAppInfo does not support add_supports_type");
  return FALSE;
}

static gboolean
_app_info_can_remove_supports_type (GAppInfo *appinfo)
{
  return FALSE;
}

static gboolean
_app_info_remove_supports_type (GAppInfo *appinfo,
                                const char *content_type,
                                GError **error)
{
  if (error)
    *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                          0,    /* error code */
                          "OlAppInfo does not support remove_supports_type");
  return FALSE;
}

static gboolean
_app_info_can_delete (GAppInfo *appinfo)
{
  return FALSE;
}

static gboolean
_app_info_do_delete (GAppInfo *appinfo)
{
  return FALSE;
}

OlAppInfo *
ol_app_info_new (const char *cmdline,
                 const char *name,
                 const char *icon_name,
                 enum OlAppInfoFlags flags,
                 GError **error)
{
  if (cmdline == NULL)
  {
    if (error)
      *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                            0,
                            "cmdline cannot be NULL");
    return NULL;
  }
  int cmd_index = 0;
  if (flags & OL_APP_INFO_SECOND_IS_EXEC)
    cmd_index = 1;
  gchar **argv = NULL;
  gint argc = 0;
  GError *parse_error = NULL;
  if (!g_shell_parse_argv (cmdline, &argc, &argv, &parse_error))
  {
    if (error)
      *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                            0,
                            "Cannot parse cmdline: %s", parse_error->message);
    g_error_free (parse_error);
    return NULL;
  }
  if (argc <= cmd_index)
  {
    if (error)
      *error = g_error_new (g_quark_from_string ("OSD Lyrics"),
                            0,
                            "Command name is required in cmdline");
    g_strfreev (argv);
    return NULL;
  }
  OlAppInfo *info = g_object_new (OL_TYPE_APP_INFO, NULL);
  info->binfile = _find_binfile (argv[cmd_index],
                                 (flags & OL_APP_INFO_WITH_PREFIX) != 0);
  if (info->binfile == NULL)
  {
    info->binfile = g_strdup (argv[cmd_index]);
    info->cmdline = g_strdup (cmdline);
    info->should_show = FALSE;
  }
  else
  {
    info->should_show = TRUE;
    _strv_replace (argv, cmd_index, info->binfile);
    info->cmdline = _str_join_argv (argv);
  }
  if (!name)
    name = info->binfile;
  if (!icon_name)
    icon_name = info->binfile;
  info->icon = _icon_new_from_name (icon_name);
  info->name = g_strdup (name);
  if (flags & OL_APP_INFO_PREFER_DESKTOP_FILE)
    _app_info_set_from_desktop_file (info, flags);
  return info;
}

static gchar *
_str_join_argv (gchar **argv)
{
  GString *str_builder = g_string_new ("");
  gboolean first = TRUE;
  while (argv && *argv)
  {
    if (!first)
      g_string_append_c (str_builder, ' ');
    else
      first = FALSE;
    if (_shell_arg_need_quote (*argv))
    {
      gchar *quoted = g_shell_quote (*argv);
      g_string_append (str_builder, quoted);
      g_free (quoted);
    }
    else
    {
      g_string_append (str_builder, *argv);
    }
    argv++;
  }
  return g_string_free (str_builder, FALSE);
}

static gboolean
_shell_arg_need_quote (const gchar *arg)
{
  if (arg == NULL)
    return FALSE;
  if (!strchr (arg, ' ') && !strchr (arg, '\t') && !strchr (arg, '\n') &&
      !strchr (arg, '\'') && !strchr (arg, '\"') && !strchr (arg, '\\'))
    return FALSE;
  else
    return TRUE;
}

static gchar *
_find_binfile (const gchar *binfile, gboolean match_prefix)
{
  GList *path_list = NULL;
  gchar *ret = NULL;
  if (g_path_is_absolute (binfile))
  {
    gchar *dirname = g_path_get_dirname (binfile);
    path_list = g_list_append (path_list, dirname);
    binfile += strlen (dirname);
    if (*binfile == G_DIR_SEPARATOR)
      binfile++;
  }
  else
  {
    const char *env_path = g_getenv ("PATH");
    if (!env_path)
    {
      env_path = "/bin:/usr/bin";
    }
    gchar **pathv = g_strsplit (env_path, G_SEARCHPATH_SEPARATOR_S, -1);
    gchar **pathiter;
    for (pathiter = pathv; *pathiter != NULL; pathiter++)
    {
      if (**pathiter != '\0')
        path_list = g_list_prepend (path_list, g_strdup (*pathiter));
    }
    path_list = g_list_reverse (path_list);
    g_strfreev (pathv);
  }
  gchar *filepath = _find_file_in_path_list (path_list,
                                             binfile,
                                             "",
                                             match_prefix,
                                             _file_is_executable);
  if (filepath)
  {
    ret = g_path_get_basename (filepath);
    g_free (filepath);
  }
  for (; path_list != NULL; path_list = g_list_delete_link (path_list, path_list))
    g_free (path_list->data);
  return ret;
}

/**
 * 
 * 
 * @param path_list A GList of gchar*.
 * @param prefix The prefix of the filename, cannot be #NULL.
 * @param suffix The suffix of filename. If #match_prefix is #TRUE, #NULL or
 *               empty string means any suffix is acceptable.
 * @param match_prefix 
 * 
 * @return 
 */
static gchar *
_find_file_in_path_list (GList *path_list,
                         const gchar *prefix,
                         const gchar *suffix,
                         gboolean match_prefix,
                         gboolean (*file_test_func) (const gchar *))
{
  ol_assert_ret (prefix != NULL, NULL);
  if (suffix == NULL)
    suffix = "";
  if (!file_test_func)
    file_test_func = _file_exists;
  GList *pathiter;
  gchar *ret = NULL;
  for (pathiter = path_list; pathiter != NULL; pathiter = g_list_next (pathiter))
  {
    gchar *path = pathiter->data;
    if (!match_prefix)
    {
      gchar *filename = NULL;
      gchar *fullname = g_strdup_printf ("%s%s", prefix, suffix);
      filename = g_build_filename (path, fullname, NULL);
      g_free (fullname);
      if (file_test_func (filename))
        ret = filename;
      else
        g_free (filename);
    } /* if !match_prefix */
    else
    {
      GError *error = NULL;
      GDir *dir = g_dir_open (path, 0, &error);
      if (!dir)
      {
        ol_errorf ("Cannot open path %s: %s\n", path, error->message);
        g_error_free (error);
        continue;
      }
      const gchar *name;
      while ((name = g_dir_read_name (dir)) != NULL)
      {
        if (g_str_has_prefix (name, prefix) && g_str_has_suffix (name, suffix))
        {
          gchar *filename = g_build_filename (path, name, NULL);
          if (file_test_func (filename) &&
              (!ret || strcmp (filename, ret) < 0))
          {
            g_free (ret);
            ret = filename;
          }
          else
          {
            g_free (filename);
          }
        }
      }
      g_dir_close (dir);
    }
    if (ret != NULL) break;
  }
  return ret;
}

static gboolean
_file_is_executable (const gchar *filename)
{
  return (g_file_test (filename, G_FILE_TEST_IS_EXECUTABLE) &&
          !g_file_test (filename, G_FILE_TEST_IS_DIR));
}

static gboolean
_file_exists (const gchar *filename)
{
  return (g_file_test (filename, G_FILE_TEST_EXISTS) &&
          !g_file_test (filename, G_FILE_TEST_IS_DIR));
}

static GIcon *
_icon_new_from_name (const gchar *icon_name)
{
  ol_assert_ret (icon_name != NULL, NULL);
  /* This is taken from gdesktopappinfo.c of GIO */
  GIcon *icon = NULL;
  if (g_path_is_absolute (icon_name))
  {
    GFile *file;
    file = g_file_new_for_path (icon_name);
    icon = g_file_icon_new (file);
    g_object_unref (file);
  }
  else
  {
    char *p;
    /* Work around a common mistake in desktop files */
    if ((p = strrchr (icon_name, '.')) != NULL &&
        (strcmp (p, ".png") == 0 ||
         strcmp (p, ".xpm") == 0 ||
         strcmp (p, ".svg") == 0))
    {
      gchar *real_name = g_strndup (icon_name, p - icon_name);
      icon = g_themed_icon_new (real_name);
      g_free (real_name);
    }
    else
    {
      icon = g_themed_icon_new (icon_name);
    }
  }
  return icon;
}

static void
_app_info_set_from_desktop_file (OlAppInfo *info,
                                 enum OlAppInfoFlags flags)
{
  GList *path_list = _get_desktop_file_path_list ();
  gchar *filename = _find_file_in_path_list (path_list,
                                             info->binfile,
                                             ".desktop",
                                             (flags & OL_APP_INFO_WITH_PREFIX) != 0,
                                             NULL);
  if (!filename)
  {
    ol_debugf ("Cannot find desktop file for %s\n", info->binfile);
    return;
  }
  GKeyFile *keyfile = g_key_file_new ();
  GError *error = NULL;
  if (!g_key_file_load_from_file (keyfile, filename, G_KEY_FILE_NONE, &error))
  {
    ol_errorf ("Cannot open desktop file %s: %s\n", filename, error->message);
    g_error_free (error);
  }
  else
  {
    if (flags & OL_APP_INFO_USE_DESKTOP_NAME)
    {
      gchar *name = g_key_file_get_locale_string (keyfile,
                                                  G_KEY_FILE_DESKTOP_GROUP,
                                                  G_KEY_FILE_DESKTOP_KEY_NAME,
                                                  NULL,
                                                  NULL);
      if (name != NULL)
      {
        if (info->name != NULL)
          g_free (info->name);
        info->name = name;
      }
    }
    if (flags & OL_APP_INFO_USE_DESKTOP_CMDLINE)
    {
      gchar *cmdline = g_key_file_get_locale_string (keyfile,
                                                     G_KEY_FILE_DESKTOP_GROUP,
                                                     G_KEY_FILE_DESKTOP_KEY_EXEC,
                                                     NULL,
                                                     NULL);
      if (cmdline != NULL)
      {
        if (info->cmdline != NULL)
          g_free (info->cmdline);
        info->cmdline = cmdline;
      }
    }
    if (flags & OL_APP_INFO_USE_DESKTOP_ICON)
    {
      gchar *icon_name = g_key_file_get_locale_string (keyfile,
                                                       G_KEY_FILE_DESKTOP_GROUP,
                                                       G_KEY_FILE_DESKTOP_KEY_ICON,
                                                       NULL,
                                                       NULL);
      GIcon *icon = _icon_new_from_name (icon_name);
      if (icon != NULL)
      {
        if (info->icon != NULL)
          g_object_unref (info->icon);
        info->icon = icon;
      }
    }
    info->should_show = !g_key_file_get_boolean (keyfile,
                                                 G_KEY_FILE_DESKTOP_GROUP,
                                                 G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY,
                                                 NULL);
  }
  g_free (filename);
  g_key_file_free (keyfile);
}

static void
ol_app_info_finalize (GObject *object)
{
  OlAppInfo *info = OL_APP_INFO (object);
  if (info->name)
  {
    g_free (info->name);
    info->name = NULL;
  }
  if (info->cmdline)
  {
    g_free (info->cmdline);
    info->cmdline = NULL;
  }
  if (info->binfile)
  {
    g_free (info->binfile);
    info->binfile = NULL;
  }
  if (info->icon)
  {
    g_object_unref (info->icon);
    info->icon = NULL;
  }
  G_OBJECT_CLASS (ol_app_info_parent_class)->finalize (object);
}

static void
_strv_replace (gchar **argv, guint index, const gchar *new_value)
{
  g_free (argv[index]);
  argv[index] = g_strdup (new_value);
}

/**
 * Returns the list of paths to find desktop files.
 * 
 * 
 * @return A GList of gchar*. Should NOT be freed.
 */
static GList *
_get_desktop_file_path_list ()
{
  GOnce once = G_ONCE_INIT;
  g_once (&once, _get_desktop_file_path_list_once, NULL);
  return (GList*) once.retval;
}

static gpointer
_get_desktop_file_path_list_once (gpointer data)
{
  GList *list = NULL;
  list = g_list_prepend (list,
                         g_build_filename (g_get_user_data_dir(),
                                           "applications",
                                           NULL));
  list = _prepend_subdirs (list);
  const gchar * const * data_dirs = g_get_system_data_dirs ();
  while (*data_dirs != NULL)
  {
    list = g_list_prepend (list,
                           g_build_filename (*data_dirs, "applications", NULL));
    list = _prepend_subdirs (list);
    data_dirs++;
  }
  return g_list_reverse (list);
}

static GList *
_prepend_subdirs (GList *list)
{
  gchar *path = list->data;
  GError *error = NULL;
  GDir *dir = g_dir_open (path, 0, &error);
  if (!dir)
  {
    ol_errorf ("Cannot open dir %s: %s\n", path, error->message);
    g_error_free (error);
    return list;
  }
  const gchar *name;
  while ((name = g_dir_read_name (dir)) != NULL)
  {
    gchar *filepath = g_build_filename (path, name, NULL);
    if (g_file_test (filepath, G_FILE_TEST_IS_DIR))
    {
      list = g_list_prepend (list, filepath);
      list = _prepend_subdirs (list);
    }
    else
    {
      g_free (filepath);
    }
  }
  return list;
}
