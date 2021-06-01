/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011  Tiger Soldier
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
 * along with OSD Lyrics.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>
#include "ol_config_proxy.h"
#include "ol_consts.h"
#include "ol_debug.h"

const int DEFAULT_SYNC_TIMEOUT = 500; /* 0.5s */

#define OL_CONFIG_PROXY_GET_PRIVATE(obj) \
    ((OlConfigProxyPrivate *)((OL_CONFIG_PROXY(obj))->priv))

enum OlConfigProxySingals {
  SIGNAL_CHANGED = 0,
  LAST_SINGAL,
};

enum _GetResult {
  GET_RESULT_OK = 0,
  GET_RESULT_FAILED,
  GET_RESULT_MISSING,
};

typedef struct {
  GHashTable *temp_values;
  GVariantBuilder *default_builder;
  guint default_sync_handler;
} OlConfigProxyPrivate;

static guint _signals[LAST_SINGAL];
static OlConfigProxy *config_proxy = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (OlConfigProxy, ol_config_proxy, G_TYPE_DBUS_PROXY);

static OlConfigProxy *ol_config_proxy_new (void);
static void ol_config_proxy_finalize (GObject *object);
static void ol_config_proxy_g_signal (GDBusProxy *proxy,
                                      const gchar *sender_name,
                                      const gchar *signal_name,
                                      GVariant *parameters);
static void ol_config_proxy_value_changed_cb (OlConfigProxy *proxy,
                                              GVariant *value);
static GVariant *_str_list_to_variant (const gchar *const *value,
                                       gssize len);
static gboolean _sync_default_cb (OlConfigProxy *config);

static GVariant *
_str_list_to_variant (const gchar *const *value,
                      gssize len)
{
  if (len < 0)
    len = g_strv_length ((gchar**) value);
  GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  int i = 0;
  for (; (len < 0 || i < len) && value[i] != NULL; i++)
  {
    g_variant_builder_add (builder, "s", value[i]);
  }
  GVariant *gvalue = g_variant_new ("as", builder);
  g_variant_builder_unref (builder);
  return gvalue;
}

static gboolean
_sync_default_cb (OlConfigProxy *config)
{
  OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (config);
  priv->default_sync_handler = 0;
  if (priv->default_builder != NULL)
  {
    GError *error = NULL;
    GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (config),
                                            "SetDefaultValues",
                                            g_variant_new ("(a{sv})", priv->default_builder),
                                            G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                            -1,   /* timeout_secs */
                                            NULL, /* cancellable */
                                            &error);
    if (ret)
    {
      g_variant_unref (ret);
    }
    else
    {
      ol_errorf ("Cannot set config default values: %s\n", error->message);
      g_error_free (error);
    }
    g_variant_builder_unref (priv->default_builder);
    priv->default_builder = NULL;
  }
  return FALSE;
}

void
ol_config_proxy_sync (OlConfigProxy *config)
{
  OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (config);
  if (priv->default_builder != NULL)
  {
    g_source_remove (priv->default_sync_handler);
    _sync_default_cb (config);
  }
}

static void
ol_config_proxy_class_init (OlConfigProxyClass *klass)
{
  GObjectClass *gobject_class;
  GDBusProxyClass *proxy_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = ol_config_proxy_finalize;

  proxy_class = G_DBUS_PROXY_CLASS (klass);
  proxy_class->g_signal = ol_config_proxy_g_signal;

  _signals[SIGNAL_CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DETAILED,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);
}

static void
ol_config_proxy_init (OlConfigProxy *proxy)
{
  /* Allocate Private data structure */
  (OL_CONFIG_PROXY(proxy))->priv = \
    (OlConfigProxyPrivate *) g_malloc0(sizeof(OlConfigProxyPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_CONFIG_PROXY(proxy))->priv != NULL)
  {
    OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (proxy);
    priv->temp_values = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               g_free,
                                               (GDestroyNotify) g_variant_unref);
  }
}

static void
ol_config_proxy_dispose(GObject *object)
{
  OlConfigProxy *self = (OlConfigProxy *)object;
  OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE(self);
  /* Check if not NULL! To avoid calling dispose multiple times */
  if(priv != NULL)
  {
    /* Deallocate contents of the private data, if any */
    /* Deallocate private data structure */
    g_free(priv);
    /* And finally set the opaque pointer back to NULL, so that
     *  we don't deallocate it twice. */
    (OL_CONFIG_PROXY(self))->priv = NULL;
  }
}

static void
ol_config_proxy_finalize (GObject *object)
{
  OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (object);
  g_hash_table_destroy (priv->temp_values);
  priv->temp_values = NULL;
  ol_config_proxy_sync (OL_CONFIG_PROXY (object));
}

static OlConfigProxy *
ol_config_proxy_new (void)
{
  GInitable *ret;
  ret = g_initable_new (OL_TYPE_CONFIG_PROXY,
                        NULL,   /* cancellable */
                        NULL,
                        "g-flags", G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                        "g-name", OL_SERVICE_CONFIG,
                        "g-bus-type", G_BUS_TYPE_SESSION,
                        "g-object-path", OL_OBJECT_CONFIG,
                        "g-interface-name", OL_IFACE_CONFIG,
                        NULL);
  if (ret != NULL)
    return OL_CONFIG_PROXY (ret);
  else
    return NULL;
}

static void
ol_config_proxy_g_signal (GDBusProxy *proxy,
                          const gchar *sender_name,
                          const gchar *signal_name,
                          GVariant *parameters)
{
  if (strcmp (signal_name, "ValueChanged") == 0)
  {
    ol_config_proxy_value_changed_cb (OL_CONFIG_PROXY (proxy),
                                      parameters);
  }
  else
  {
    ol_errorf ("Unknown D-Bus signal: %s\n", signal_name);
  }
}

static void
ol_config_proxy_value_changed_cb (OlConfigProxy *proxy,
                                  GVariant *parameters)
{
  GVariantIter *iter = NULL;
  g_variant_get (parameters, "(as)", &iter);
  gchar *name = NULL;
  while (g_variant_iter_loop (iter, "s", &name))
  {
    g_signal_emit (proxy,
                   _signals[SIGNAL_CHANGED],
                   g_quark_from_string (name),
                   name);
  }
  g_variant_iter_free (iter);
}

OlConfigProxy*
ol_config_proxy_get_instance (void)
{
  if (config_proxy == NULL)
    config_proxy = ol_config_proxy_new ();
  return config_proxy;
}

void
ol_config_proxy_unload (void)
{
  if (config_proxy != NULL)
  {
    g_object_unref (config_proxy);
    config_proxy = NULL;
  }
}

static gboolean
ol_config_proxy_set (OlConfigProxy *config,
                     const gchar *method,
                     const gchar *key,
                     GVariant *value)
{
  ol_assert_ret (key != NULL && key[0] != '\0', FALSE);
  if (key[0] == '.')
  {
    OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (config);
    g_variant_ref_sink (value);
    g_hash_table_insert (priv->temp_values,
                         g_strdup (key),
                         g_variant_new ("(*)", g_variant_ref_sink (value)));
    g_signal_emit (config,
                   _signals[SIGNAL_CHANGED],
                   g_quark_from_string (key),
                   key);
    g_variant_unref (value);
    return TRUE;
  }
  else
  {
    GError *error = NULL;
    GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (config),
                                            method,
                                            g_variant_new ("(s*)", key, value),
                                            G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                            -1,   /* timeout_secs */
                                            NULL, /* cancellable */
                                            &error);
    if (ret)
    {
      g_variant_unref (ret);
      return TRUE;
    }
    else
    {
      ol_errorf ("Cannot set config value: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }
  }
}

gboolean
ol_config_proxy_set_bool (OlConfigProxy *config,
                          const gchar *key,
                          gboolean value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  return ol_config_proxy_set (config,
                              "SetBool",
                              key,
                              g_variant_new ("b", value));
}

gboolean
ol_config_proxy_set_int (OlConfigProxy *config,
                         const gchar *key,
                         gint value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  return ol_config_proxy_set (config,
                              "SetInt",
                              key,
                              g_variant_new ("i", value));
}

gboolean
ol_config_proxy_set_double (OlConfigProxy *config,
                            const gchar *key,
                            gdouble value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  return ol_config_proxy_set (config,
                              "SetDouble",
                              key,
                              g_variant_new ("d", value));
}

gboolean
ol_config_proxy_set_string (OlConfigProxy *config,
                            const gchar *key,
                            const gchar* value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  ol_assert_ret (value != NULL, FALSE);
  return ol_config_proxy_set (config,
                              "SetString",
                              key,
                              g_variant_new ("s", value));
}

gboolean
ol_config_proxy_set_str_list (OlConfigProxy *config,
                              const gchar *key,
                              const gchar * const *value,
                              gint len)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  ol_assert_ret (value != NULL, FALSE);
  return ol_config_proxy_set (config,
                              "SetStringList",
                              key,
                              _str_list_to_variant (value, len));
}

static gboolean
ol_config_proxy_set_default (OlConfigProxy *config,
                             const gchar *key,
                             GVariant *value)
{
  ol_assert_ret (key != NULL && key[0] != '\0', FALSE);
  OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (config);
  if (key[0] == '.' && !g_hash_table_lookup (priv->temp_values, key))
  {
    g_variant_ref_sink (value);
    g_hash_table_insert (priv->temp_values,
                         g_strdup (key),
                         g_variant_new ("(*)", g_variant_ref_sink (value)));
    g_signal_emit (config,
                   _signals[SIGNAL_CHANGED],
                   g_quark_from_string (key),
                   key);
    g_variant_unref (value);
  }
  else
  {
    if (priv->default_builder == NULL)
    {
      priv->default_builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
      priv->default_sync_handler = g_timeout_add (DEFAULT_SYNC_TIMEOUT,
                                                  (GSourceFunc) _sync_default_cb,
                                                  config);
    }
    g_variant_builder_add (priv->default_builder, "{sv}", key, value);
  }
  return TRUE;
}

gboolean
ol_config_proxy_set_bool_default (OlConfigProxy *config,
                                  const gchar *key,
                                  gboolean value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  return ol_config_proxy_set_default (config,
                                      key,
                                      g_variant_new ("b", value));
}

gboolean
ol_config_proxy_set_int_default (OlConfigProxy *config,
                                 const gchar *key,
                                 gint value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  return ol_config_proxy_set_default (config,
                                      key,
                                      g_variant_new ("i", value));
}

gboolean
ol_config_proxy_set_double_default (OlConfigProxy *config,
                                    const gchar *key,
                                    gdouble value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  return ol_config_proxy_set_default (config,
                                      key,
                                      g_variant_new ("d", value));
}

gboolean
ol_config_proxy_set_string_default (OlConfigProxy *config,
                                    const gchar *key,
                                    const gchar* value)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  ol_assert_ret (value != NULL, FALSE);
  return ol_config_proxy_set_default (config,
                                      key,
                                      g_variant_new ("s", value));
}

gboolean
ol_config_proxy_set_str_list_default (OlConfigProxy *config,
                                      const gchar *key,
                                      const gchar * const *value,
                                      gint len)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), FALSE);
  ol_assert_ret (key != NULL, FALSE);
  ol_assert_ret (value != NULL, FALSE);
  return ol_config_proxy_set_default (config,
                                      key,
                                      _str_list_to_variant (value, len));
}

static enum _GetResult
ol_config_proxy_get (OlConfigProxy *config,
                     const gchar *method,
                     const gchar *key,
                     const gchar *format_string,
                     gpointer retval)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), GET_RESULT_FAILED);
  ol_assert_ret (key != NULL, GET_RESULT_FAILED);
  enum _GetResult ret = GET_RESULT_OK;
  GError *error = NULL;
  GVariant *value = NULL;
  OlConfigProxyPrivate *priv = OL_CONFIG_PROXY_GET_PRIVATE (config);
  if (key[0] == '.')
  {
    value = g_hash_table_lookup (priv->temp_values, key);
    if (value)
      g_variant_ref (value);
  }
  else
  {
    value = g_dbus_proxy_call_sync (G_DBUS_PROXY (config),
                                    method,
                                    g_variant_new ("(s)", key),
                                    G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                    -1,   /* timeout_secs */
                                    NULL, /* cancellable */
                                    &error);
  }
  if (!value)
  {
    if (g_dbus_error_is_remote_error (error))
    {
      gchar *error_name = g_dbus_error_get_remote_error (error);
      if (strcmp (error_name, OL_ERROR_VALUE_NOT_EXIST) == 0)
      {
        ol_debugf ("Key %s not exists, use default value\n", key);
        ret = GET_RESULT_MISSING;
      }
      else
      {
        ol_errorf ("Failed to get config %s: %s\n", key, error->message);
        ret = GET_RESULT_FAILED;
      }
      g_free (error_name);
    }
    else
    {
      ol_errorf ("%s failed. Cannot get value %s from config: %s\n",
                 method, key, error->message);
      ret = GET_RESULT_FAILED;
    }
    g_error_free (error);
  }
  else
  {
    g_variant_get (value, format_string, retval);
    g_variant_unref (value);
  }
  return ret;
}

gboolean
ol_config_proxy_get_bool (OlConfigProxy *config,
                          const gchar *key)
{
  gboolean ret;
  switch (ol_config_proxy_get (config, "GetBool", key, "(b)", &ret))
  {
  case GET_RESULT_OK:
    return ret;
  case GET_RESULT_MISSING:
  case GET_RESULT_FAILED:
    return FALSE;
  default:
    ol_error ("Unknown return value from ol_config_proxy_get");
    return FALSE;
  }
}

gint
ol_config_proxy_get_int (OlConfigProxy *config,
                         const gchar *key)
{
  gint ret;
  switch (ol_config_proxy_get (config, "GetInt", key, "(i)", &ret))
  {
  case GET_RESULT_OK:
    return ret;
  case GET_RESULT_MISSING:
  case GET_RESULT_FAILED:
    return 0;
  default:
    ol_error ("Unknown return value from ol_config_proxy_get");
    return 0;
  }
}

gdouble
ol_config_proxy_get_double (OlConfigProxy *config,
                            const gchar *key)
{
  gdouble ret;
  switch (ol_config_proxy_get (config, "GetDouble", key, "(d)", &ret))
  {
  case GET_RESULT_OK:
    return ret;
  case GET_RESULT_MISSING:
  case GET_RESULT_FAILED:
    return 0.0;
  default:
    ol_error ("Unknown return value from ol_config_proxy_get");
    return 0.0;
  }
}

gchar*
ol_config_proxy_get_string (OlConfigProxy *config,
                            const gchar *key)
{
  gchar *ret;
  switch (ol_config_proxy_get (config, "GetString", key, "(s)", &ret))
  {
  case GET_RESULT_OK:
    return ret;
  case GET_RESULT_MISSING:
    return NULL;
  default:
    ol_error ("Unknown return value from ol_config_proxy_get");
    return NULL;
  }
}

gchar**
ol_config_proxy_get_str_list (OlConfigProxy *config,
                              const char *key,
                              gsize *len)
{
  ol_assert_ret (OL_IS_CONFIG_PROXY (config), NULL);
  ol_assert_ret (key != NULL, NULL);
  GError *error = NULL;
  GVariant *value = g_dbus_proxy_call_sync (G_DBUS_PROXY (config),
                                            "GetStringList",
                                            g_variant_new ("(s)", key),
                                            G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                            -1,   /* timeout_secs */
                                            NULL, /* cancellable */
                                            &error);
  if (!value)
  {
    ol_errorf ("%s failed. Cannot get value %s from config: %s\n",
               "GetStringList", key, error->message);
    g_error_free (error);
    return NULL;
  }
  else
  {
    GVariantIter *iter = NULL;
    g_variant_get (value, "(as)", &iter);
    gsize list_size = g_variant_iter_n_children (iter);
    gchar **retval = g_new (gchar*, list_size + 1);
    gchar **ret_iter = retval;
    while (g_variant_iter_next (iter, "s", ret_iter))
      ret_iter++;
    retval[list_size] = NULL;
    g_variant_unref (value);
    g_variant_iter_free (iter);
    if (len)
      *len = list_size;
    return retval;
  }
}
