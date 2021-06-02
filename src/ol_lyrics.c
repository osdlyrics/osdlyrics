#include <string.h>
#include "ol_lyrics.h"
#include "ol_consts.h"
#include "ol_debug.h"

typedef struct _OlLyricsPrivate OlLyricsPrivate;
struct _OlLyricsPrivate
{
  int padding;
};

enum {
  LYRICS_CHANGED_SIGNAL = 0,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

#define OL_LYRICS_GET_PRIVATE(object) \
    ((OlLyricsPrivate *)((OL_LYRICS(object))->priv))

static void ol_lyrics_finalize (GObject *object);
static void ol_lyrics_g_signal (GDBusProxy *proxy,
                                const gchar *sender_name,
                                const gchar *signal_name,
                                GVariant *parameters);
static OlLrc *ol_lyrics_get_lrc_from_variant (OlLyrics *proxy,
                                              GVariant *variant);

G_DEFINE_TYPE_WITH_PRIVATE (OlLyrics, ol_lyrics, G_TYPE_DBUS_PROXY);

static void
ol_lyrics_init (OlLyrics *proxy)
{
}

static void
ol_lyrics_finalize (GObject *object)
{
  G_OBJECT_CLASS (ol_lyrics_parent_class)->finalize (object);
}

static void
ol_lyrics_class_init (OlLyricsClass *klass)
{
  GObjectClass *gobject_class;
  GDBusProxyClass *proxy_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize     = ol_lyrics_finalize;
  /* gobject_class->get_property = org_osdlyrics_lyrics_proxy_get_property; */
  /* gobject_class->set_property = org_osdlyrics_lyrics_proxy_set_property; */

  proxy_class = G_DBUS_PROXY_CLASS (klass);
  proxy_class->g_signal = ol_lyrics_g_signal;
  /* proxy_class->g_properties_changed = ol_lyrics_g_properties_changed; */
  signals[LYRICS_CHANGED_SIGNAL] =
    g_signal_new ("lyrics-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,            /* class_offset */
                  NULL, NULL,   /* accumulator, accu_data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

static void
ol_lyrics_g_signal (GDBusProxy *proxy,
                    const gchar *sender_name,
                    const gchar *signal_name,
                    GVariant *parameters)
{
  if (strcmp (signal_name, "CurrentLyricsChanged") == 0)
  {
    g_signal_emit (proxy,
                   signals[LYRICS_CHANGED_SIGNAL],
                   0);
  }
  else
  {
    ol_errorf ("Unknown D-Bus signal: %s\n", signal_name);
  }
}

OlLyrics *
ol_lyrics_new (GError **error)
{
  GInitable *ret;
  ret = g_initable_new (OL_TYPE_LYRICS,
                        NULL,   /* cancellable */
                        error,
                        "g-flags", G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                        "g-name", OL_SERVICE_DAEMON,
                        "g-bus-type", G_BUS_TYPE_SESSION,
                        "g-object-path", OL_OBJECT_LYRICS,
                        "g-interface-name", OL_IFACE_LYRICS,
                        NULL);
  if (ret != NULL)
    return OL_LYRICS (ret);
  else
    return NULL;
}

void
ol_lyrics_proxy_new_async (GCancellable *cancellable,
                           GAsyncReadyCallback callback,
                           gpointer user_data,
                           GError **error)
{
  g_async_initable_new_async (OL_TYPE_LYRICS,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "g-flags", G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                              "g-name", OL_SERVICE_DAEMON,
                              "g-bus-type", G_BUS_TYPE_SESSION,
                              "g-object-path", OL_OBJECT_LYRICS,
                              "g-interface-name", OL_IFACE_LYRICS,
                              NULL);
}

OlLyrics *
ol_lyrics_proxy_new_finish (GAsyncResult *res,
                            GError **error)
{
  GObject *ret;
  GObject *source_object;
  source_object = g_async_result_get_source_object (res);
  ret = g_async_initable_new_finish (G_ASYNC_INITABLE (source_object), res, error);
  g_object_unref (source_object);
  if (ret != NULL)
    return OL_LYRICS (ret);
  else
    return NULL;
}

static OlLrc *
ol_lyrics_get_lrc_from_variant (OlLyrics *proxy, GVariant *variant)
{
  OlLrc *lrc = NULL;
  gboolean found;
  gchar *uri;
  GVariant *metadata, *content;
  g_variant_get (variant, "(bs@a{ss}@aa{sv})", &found, &uri, &metadata, &content);
  if (found)
  {
    lrc = ol_lrc_new (proxy, uri);
    ol_lrc_set_attributes_from_variant (lrc, metadata);
    ol_lrc_set_content_from_variant (lrc, content);
  }
  g_free (uri);
  g_variant_unref (content);
  g_variant_unref (metadata);
  return lrc;
}

OlLrc *
ol_lyrics_get_current_lyrics (OlLyrics *proxy)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), NULL);
  GError *error = NULL;
  OlLrc *lrc = NULL;
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "GetCurrentLyrics",
                                          NULL,      /* parameters */
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          &error);
  if (ret)
  {
    lrc = ol_lyrics_get_lrc_from_variant (proxy, ret);
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Cannot get current lyrics from daemon: %s\n", error->message);
    g_error_free (error);
  }
  return lrc;
}

OlLrc *
ol_lyrics_get_lyrics (OlLyrics *proxy,
                      OlMetadata *metadata)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), NULL);
  ol_assert_ret (metadata != NULL, NULL);
  GError *error = NULL;
  OlLrc *lrc = NULL;
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "GetLyrics",
                                          g_variant_new ("(@a{sv})",
                                                         ol_metadata_to_variant (metadata)),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          &error);
  if (ret)
  {
    lrc = ol_lyrics_get_lrc_from_variant (proxy, ret);
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Cannot get lyrics from daemon: %s\n", error->message);
    g_error_free (error);
  }
  return lrc;
}

static gboolean
ol_lyrics_get_raw_lyrics_from_variant (GVariant *variant,
                                       char **uri,
                                       char **content)
{
  gchar *vuri, *vcontent;
  gboolean found;
  g_variant_get (variant, "(bss)", &found, &vuri, &vcontent);
  if (found)
  {
    if (uri)
      *uri = vuri;
    else
      g_free (vuri);
    if (content)
      *content = vcontent;
    else
      g_free (vcontent);
  }
  else
  {
    g_free (vuri);
    g_free (vcontent);
    if (uri)
      *uri = NULL;
    if (content)
      *content = NULL;
  }
  return found;
}

gboolean
ol_lyrics_get_raw_lyrics (OlLyrics *proxy,
                          OlMetadata *metadata,
                          char **uri,
                          char **content)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), FALSE);
  ol_assert_ret (metadata != NULL, FALSE);
  GError *error = NULL;
  gboolean found = FALSE;
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "GetRawLyrics",
                                          g_variant_new ("(@a{sv})",
                                                         ol_metadata_to_variant (metadata)),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          &error);
  if (ret)
  {
    found = ol_lyrics_get_raw_lyrics_from_variant (ret, uri, content);
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Cannot get lyrics from daemon: %s\n", error->message);
    g_error_free (error);
  }
  return found;
}

gboolean
ol_lyrics_get_current_raw_lyrics (OlLyrics *proxy,
                                  char **uri,
                                  char **content)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), FALSE);
  GError *error = NULL;
  gboolean found = FALSE;
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "GetCurrentRawLyrics",
                                          NULL,      /* parameters */
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          &error);
  if (ret)
  {
    found = ol_lyrics_get_raw_lyrics_from_variant (ret, uri, content);
    g_variant_unref (ret);
  }
  else
  {
    ol_errorf ("Cannot get lyrics from daemon: %s\n", error->message);
    g_error_free (error);
  }
  return found;
}

gchar *
ol_lyrics_set_content (OlLyrics *proxy,
                       OlMetadata *metadata,
                       const char *content,
                       GError **error)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), NULL);
  ol_assert_ret (metadata != NULL, NULL);
  ol_assert_ret (content != NULL, NULL);
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "SetLyricContent",
                                          g_variant_new ("(@a{sv}@ay)",
                                                         ol_metadata_to_variant (metadata),
                                                         g_variant_new_bytestring (content)),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          error);
  if (ret)
  {
    gchar *uri = NULL;
    g_variant_get (ret, "(s)", &uri);
    g_variant_unref (ret);
    return uri;
  }
  else
  {
    return NULL;
  }
}

gboolean
ol_lyrics_assign (OlLyrics *proxy,
                  OlMetadata *metadata,
                  const char *uri,
                  GError **error)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), FALSE);
  ol_assert_ret (metadata != NULL, FALSE);
  gboolean result = FALSE;
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "AssignLyricFile",
                                          g_variant_new ("(@a{sv}s)",
                                                         ol_metadata_to_variant (metadata),
                                                         uri),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          error);
  if (ret)
  {
    result = TRUE;
    g_variant_unref (ret);
  }
  return result;
}

gboolean
ol_lyrics_set_offset (OlLyrics *proxy,
                      const char *uri,
                      gint offset)
{
  ol_assert_ret (OL_IS_LYRICS (proxy), FALSE);
  ol_assert_ret (uri != NULL, FALSE);
  GError *error = NULL;
  GVariant *ret = g_dbus_proxy_call_sync (G_DBUS_PROXY (proxy),
                                          "SetOffset",
                                          g_variant_new ("(si)",
                                                         uri,
                                                         offset),
                                          G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                          -1,        /* timeout_secs */
                                          NULL,      /* cancellable */
                                          &error);
  if (ret)
  {
    g_variant_unref (ret);
    return TRUE;
  }
  else
  {
    ol_errorf ("Failed to set offset: %s\n", error->message);
    g_error_free (error);
    return FALSE;
  }
}
