/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldier@gmail.com>
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
#include <stdio.h>
#include "ol_utils_dbus.h"
#include "ol_debug.h"

static const char *DBUS_NAME = "org.freedesktop.DBus";
static const char *DBUS_IFACE = "org.freedesktop.DBus";
static const char *DBUS_PATH = "/";
static const char *DBUS_LIST_NAMES = "ListNames";

static DBusGConnection *connection = NULL;
static GError *error = NULL;

DBusGConnection
*ol_dbus_get_connection ()
{
  if (connection == NULL)
  {
    connection = dbus_g_bus_get (DBUS_BUS_SESSION,
                                 &error);
    if (connection == NULL)
    {
      ol_debugf ("get connection failed: %s\n", error->message);
      g_error_free(error);
      error = NULL;
    }
  }
  return connection;
}

gboolean
ol_dbus_get_string (DBusGProxy *proxy, const gchar *method, gchar **returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  if (dbus_g_proxy_call (proxy,
                         method,
                         NULL,
                         G_TYPE_INVALID,
                         G_TYPE_STRING,
                         returnval,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

gboolean
ol_dbus_get_string_with_str_arg (DBusGProxy *proxy, const gchar *method, const gchar *arg, gchar **returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  ol_assert_ret (arg != NULL, FALSE);
  if (dbus_g_proxy_call (proxy,
                         method,
                         NULL,
                         G_TYPE_STRING,
                         arg,
                         G_TYPE_INVALID,
                         G_TYPE_STRING,
                         returnval,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

gboolean
ol_dbus_set_string (DBusGProxy *proxy,
                    const gchar *method,
                    const gchar *value)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (value != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_STRING,
                            value,
                            G_TYPE_INVALID,
                            G_TYPE_INVALID);
}

gboolean
ol_dbus_get_uint (DBusGProxy *proxy, const gchar *method, guint *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  GError *error = NULL;
  if (dbus_g_proxy_call (proxy,
                         method,
                         &error,
                         G_TYPE_INVALID,
                         G_TYPE_UINT,
                         returnval,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    ol_errorf ("call %s failed: %s\n", method, error->message);
    g_error_free (error);
    return FALSE;
  }
}

gboolean
ol_dbus_set_uint (DBusGProxy *proxy,
                  const gchar *method,
                  guint value)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  GError *error = NULL;
  if (dbus_g_proxy_call (proxy,
                         method,
                         &error,
                         G_TYPE_UINT,
                         value,
                         G_TYPE_INVALID,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    ol_errorf ("call %s failed: %s\n", method, error->message);
    g_error_free (error);
    return FALSE;
  }
}

gboolean
ol_dbus_get_int64 (DBusGProxy *proxy, const gchar *method, gint64 *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  GError *error = NULL;
  if (dbus_g_proxy_call (proxy,
                         method,
                         &error,
                         G_TYPE_INVALID,
                         G_TYPE_INT64,
                         returnval,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    ol_errorf ("call %s failed: %s\n", method, error->message);
    g_error_free (error);
    return FALSE;
  }
}

gboolean
ol_dbus_set_int64 (DBusGProxy *proxy,
                   const gchar *method,
                   gint64 value)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  GError *error = NULL;
  if (dbus_g_proxy_call (proxy,
                         method,
                         &error,
                         G_TYPE_INT64,
                         value,
                         G_TYPE_INVALID,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    ol_errorf ("call %s failed: %s\n", method, error->message);
    g_error_free (error);
    return FALSE;
  }
}

gboolean
ol_dbus_get_int (DBusGProxy *proxy, const gchar *method, gint *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  GError *error = NULL;
  if (dbus_g_proxy_call (proxy,
                         method,
                         &error,
                         G_TYPE_INVALID,
                         G_TYPE_INT,
                         returnval,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    ol_errorf ("call %s failed: %s\n", method, error->message);
    g_error_free (error);
    return FALSE;
  }
}

gboolean
ol_dbus_set_int (DBusGProxy *proxy,
                 const gchar *method,
                 gint value)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  GError *error = NULL;
  if (dbus_g_proxy_call (proxy,
                         method,
                         &error,
                         G_TYPE_INT,
                         value,
                         G_TYPE_INVALID,
                         G_TYPE_INVALID))
  {
    return TRUE;
  }
  else
  {
    ol_errorf ("call %s failed: %s\n", method, error->message);
    g_error_free (error);
    return FALSE;
  }
}

gboolean
ol_dbus_get_uint8 (DBusGProxy *proxy, const gchar *method, guint8 *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_INVALID,
                            G_TYPE_UCHAR,
                            returnval,
                            G_TYPE_INVALID);
}

gboolean
ol_dbus_set_uint8 (DBusGProxy *proxy,
                   const gchar *method,
                   guint8 value)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_UCHAR,
                            value,
                            G_TYPE_INVALID,
                            G_TYPE_INVALID);
}
gboolean
ol_dbus_get_double (DBusGProxy *proxy,
                   const gchar *method,
                   gdouble *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_INVALID,
                            G_TYPE_DOUBLE,
                            returnval,
                            G_TYPE_INVALID);
}

gboolean
ol_dbus_get_bool (DBusGProxy *proxy, const gchar *method, gboolean *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_INVALID,
                            G_TYPE_BOOLEAN,
                            returnval,
                            G_TYPE_INVALID);
}

gboolean
ol_dbus_set_bool (DBusGProxy *proxy,
                  const gchar *method,
                  gboolean value)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_BOOLEAN,
                            value,
                            G_TYPE_INVALID,
                            G_TYPE_INVALID);
}

gboolean
ol_dbus_invoke (DBusGProxy *proxy, const gchar *method)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_INVALID,
                            G_TYPE_INVALID);
}

gboolean
ol_dbus_invoke_with_str_arg (DBusGProxy *proxy, const gchar *method, const gchar *arg)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (method != NULL, FALSE);
  return dbus_g_proxy_call (proxy,
                            method,
                            NULL,
                            G_TYPE_STRING,
                            arg,
                            G_TYPE_INVALID,
                            G_TYPE_INVALID);
}
gboolean
ol_dbus_connect (const gchar *service,
                 const gchar *path,
                 const gchar *interface,
                 GCallback disconnect_handler,
                 gpointer disconnect_data,
                 DBusGProxy **proxy)
{
  ol_log_func ();
  GError *error = NULL;
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (service != NULL, FALSE);
  ol_assert_ret (path != NULL, FALSE);
  ol_assert_ret (interface != NULL, FALSE);
  DBusGConnection *connection = ol_dbus_get_connection ();
  if (connection == NULL)
    return FALSE;
  *proxy = dbus_g_proxy_new_for_name_owner (connection, service, path, interface, &error);
  if (*proxy == NULL)
  {
    ol_debugf ("get proxy failed: %s\n", error->message);
    g_error_free (error);
    error = NULL;
    return FALSE;
  }
  if (disconnect_handler != NULL)
  {
    if (disconnect_data == NULL)
      disconnect_data = (gpointer) proxy;
    g_signal_connect (*proxy,
                      "destroy",
                      G_CALLBACK (disconnect_handler),
                      disconnect_data);
  }
  return TRUE;
}

void
ol_dbus_unref_proxy (GObject *object, DBusGProxy **proxy)
{
  ol_log_func ();
  if (*proxy != NULL)
  {
    g_object_unref (*proxy);
    *proxy = NULL;
  }
}

char**
ol_dbus_list_names ()
{
  DBusGConnection *connection = ol_dbus_get_connection ();
  ol_assert_ret (connection != NULL, NULL);
  GError *error = NULL;
  DBusGProxy *proxy = dbus_g_proxy_new_for_name (connection,
                                                 DBUS_NAME,
                                                 DBUS_PATH,
                                                 DBUS_IFACE);
  ol_assert_ret (proxy != NULL, NULL);
  char **names = NULL;
  if (!dbus_g_proxy_call (proxy,
                          DBUS_LIST_NAMES,
                          &error,
                          G_TYPE_INVALID,
                          G_TYPE_STRV,
                          &names,
                          G_TYPE_INVALID))
  {
    ol_errorf ("Cannot list names of session bus: %s\n", error->message);
    g_error_free (error);
  }
  g_object_unref (proxy);
  return names;
}

gboolean
ol_dbus_get_property (DBusGProxy *proxy,
                      const char *name,
                      GValue *returnval)
{
  ol_assert_ret (proxy != NULL, FALSE);
  ol_assert_ret (name != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  const char *iface = dbus_g_proxy_get_interface (proxy);
  GError *error = NULL;
  gboolean ret = TRUE;
  DBusGProxy *dbus_proxy = dbus_g_proxy_new_from_proxy (proxy,
                                                        "org.freedesktop.DBus.Properties",
                                                        NULL);
  if (!dbus_g_proxy_call (dbus_proxy, "Get", &error,
                          G_TYPE_STRING, iface,
                          G_TYPE_STRING, name,
                          G_TYPE_INVALID,
                          G_TYPE_VALUE, returnval,
                          G_TYPE_INVALID))
  {
    ret = FALSE;
    ol_debugf ("Get dbus property %s.%s failed: %s\n",
               iface, name, error->message);
    g_error_free (error);
  }
  g_object_unref (dbus_proxy);
  return ret;
}

gboolean
ol_dbus_get_bool_property (DBusGProxy *proxy,
                           const char *name,
                           gboolean *returnval)
{
  ol_assert_ret (returnval != NULL, FALSE);
  GValue value = {0};
  gboolean ret = TRUE;
  if (!ol_dbus_get_property (proxy, name, &value))
  {
    ret = FALSE;
  }
  else
  {
    if (!G_VALUE_HOLDS_BOOLEAN (&value))
    {
      ol_errorf ("Property type mismatch, %s got\n", G_VALUE_TYPE_NAME (&value));
      ret = FALSE;
    }
    else
    {
      *returnval = g_value_get_boolean (&value);
    }
    g_value_unset (&value);
  }
  return ret;
}

gboolean
ol_dbus_get_int64_property (DBusGProxy *proxy,
                            const char *name,
                            gint64 *returnval)
{
  ol_assert_ret (returnval != NULL, FALSE);
  GValue value = {0};
  gboolean ret = TRUE;
  if (!ol_dbus_get_property (proxy, name, &value))
  {
    ret = FALSE;
  }
  else
  {
    if (!G_VALUE_HOLDS_INT64 (&value))
    {
      ol_errorf ("Property type mismatch, %s got\n", G_VALUE_TYPE_NAME (&value));
      ret = FALSE;
    }
    else
    {
      *returnval = g_value_get_int64 (&value);
    }
    g_value_unset (&value);
  }
  return ret;
}

gboolean
ol_dbus_get_string_property (DBusGProxy *proxy,
                             const char *name,
                             char **returnval)
{
  ol_assert_ret (returnval != NULL, FALSE);
  GValue value = {0};
  gboolean ret = TRUE;
  if (!ol_dbus_get_property (proxy, name, &value))
  {
    ret = FALSE;
  }
  else
  {
    if (!G_VALUE_HOLDS_STRING (&value))
    {
      ol_errorf ("Property type mismatch, %s got\n", G_VALUE_TYPE_NAME (&value));
      ret = FALSE;
    }
    else
    {
      *returnval = g_strdup (g_value_get_string (&value));
    }
    g_value_unset (&value);
  }
  return ret;
}

gboolean
ol_dbus_get_dict_property (DBusGProxy *proxy,
                           const char *name,
                           GHashTable **returnval)
{
  ol_assert_ret (returnval != NULL, FALSE);
  GValue value = {0};
  gboolean ret = TRUE;
  if (!ol_dbus_get_property (proxy, name, &value))
  {
    ret = FALSE;
  }
  else
  {
    if (!G_VALUE_HOLDS_BOXED (&value))
    {
      ol_errorf ("Property type mismatch, %s got\n", G_VALUE_TYPE_NAME (&value));
      ret = FALSE;
    }
    else
    {
      *returnval = g_value_get_boxed (&value);
      g_hash_table_ref (*returnval);
    }
    g_value_unset (&value);
  }
  return ret;
}
