/* GTK3 migration regressions. Run under X11 (for example xvfb-run). */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/extensions/shape.h>
#include "ol_osd_window.h"
#include "ol_scroll_window.h"
#include "ol_image_button.h"
#include "ol_app_chooser_widget.h"
#include "ol_cell_renderer_button.h"
#include "ol_player_chooser.h"

static void
flush_events (void)
{
  for (int i = 0; i < 10; i++)
  {
    while (gtk_events_pending ())
      gtk_main_iteration ();
    g_usleep (10000);
  }
}

static cairo_surface_t *
render_widget (GtkWidget *widget)
{
  flush_events ();
  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);
  g_assert_cmpint (width, >, 0);
  g_assert_cmpint (height, >, 0);
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cairo_t *cr = cairo_create (surface);
  gtk_widget_draw (widget, cr);
  g_assert_cmpint (cairo_status (cr), ==, CAIRO_STATUS_SUCCESS);
  cairo_destroy (cr);
  cairo_surface_flush (surface);
  return surface;
}

static void
assert_has_pixels (cairo_surface_t *surface)
{
  unsigned char *data = cairo_image_surface_get_data (surface);
  int size = cairo_image_surface_get_stride (surface) * cairo_image_surface_get_height (surface);
  gboolean nonzero = FALSE;
  for (int i = 0; i < size; i++)
    nonzero |= data[i] != 0;
  g_assert_true (nonzero);
}

static void
assert_input_shape (GtkWidget *widget, gboolean locked)
{
  GdkWindow *window = gtk_widget_get_window (widget);
  int count, ordering;
  XRectangle *rectangles = XShapeGetRectangles (GDK_WINDOW_XDISPLAY (window),
                                                GDK_WINDOW_XID (window), ShapeInput,
                                                &count, &ordering);
  if (locked)
    g_assert_cmpint (count, ==, 0);
  else
    g_assert_cmpint (count, >, 0);
  if (rectangles) XFree (rectangles);
}

static void
osd_test (void)
{
  GtkWidget *widget = ol_osd_window_new ();
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  /* A child exercises the custom preferred-size/allocation/draw implementation. */
  gtk_container_add (GTK_CONTAINER (widget), gtk_button_new_with_label ("Toolbar"));
  ol_osd_window_set_width (osd, 640);
  ol_osd_window_set_line_count (osd, 2);
  ol_osd_window_set_lyric (osd, 0, "GTK3 桌面歌词");
  ol_osd_window_set_lyric (osd, 1, "Second lyric line");
  ol_osd_window_set_percentage (osd, 0, 0.5);
  ol_osd_window_set_blur_radius (osd, 2);
  gtk_widget_show_all (widget);
  cairo_surface_t *surface = render_widget (widget);
  assert_has_pixels (surface);
  cairo_surface_destroy (surface);
  assert_input_shape (widget, TRUE);
  ol_osd_window_set_width (osd, 480);
  flush_events ();
  assert_input_shape (widget, TRUE);
  ol_osd_window_set_locked (osd, FALSE);
  assert_input_shape (widget, FALSE);
  for (int i = 0; i < 4; i++)
  {
    gboolean dock = i % 2 == 0;
    ol_osd_window_set_mode (osd, dock ? OL_OSD_WINDOW_DOCK : OL_OSD_WINDOW_NORMAL);
    ol_osd_window_set_locked (osd, TRUE);
    ol_osd_window_move (osd, 100, 100);
    surface = render_widget (widget);
    assert_has_pixels (surface);
    cairo_surface_destroy (surface);
    assert_input_shape (widget, TRUE);
    XWindowAttributes attributes;
    GdkWindow *window = gtk_widget_get_window (widget);
    XGetWindowAttributes (GDK_WINDOW_XDISPLAY (window), GDK_WINDOW_XID (window), &attributes);
    g_assert_cmpint (!!attributes.override_redirect, ==, dock);
  }
  gtk_widget_destroy (widget);
}

static void
scroll_test (void)
{
  GtkWidget *widget = ol_scroll_window_new ();
  gtk_widget_show_all (widget);
  ol_scroll_window_set_text (OL_SCROLL_WINDOW (widget), "Searching lyrics…");
  cairo_surface_t *surface = render_widget (widget);
  assert_has_pixels (surface);
  cairo_surface_destroy (surface);
  GPtrArray *lyrics = g_ptr_array_new_with_free_func (g_free);
  g_ptr_array_add (lyrics, g_strdup ("First line"));
  g_ptr_array_add (lyrics, g_strdup ("第二行歌词"));
  ol_scroll_window_set_whole_lyrics (OL_SCROLL_WINDOW (widget), lyrics);
  g_ptr_array_unref (lyrics);
  ol_scroll_window_set_progress (OL_SCROLL_WINDOW (widget), 0, 0.5);
  surface = render_widget (widget);
  assert_has_pixels (surface);
  cairo_surface_destroy (surface);
  gtk_widget_destroy (widget);
}

static void
button_test (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *button = ol_image_button_new ();
  GdkPixbuf *sprite = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 64, 16);
  const guint32 colors[] = {0xff0000ff, 0x00ff00ff, 0x0000ffff, 0xffffffff};
  for (int i = 0; i < 4; i++)
  {
    GdkPixbuf *frame = gdk_pixbuf_new_subpixbuf (sprite, i * 16, 0, 16, 16);
    gdk_pixbuf_fill (frame, colors[i]);
    g_object_unref (frame);
  }
  ol_image_button_set_pixbuf (OL_IMAGE_BUTTON (button), sprite);
  gtk_container_add (GTK_CONTAINER (window), button);
  gtk_widget_show_all (window);
  const GtkStateFlags states[] = {GTK_STATE_FLAG_NORMAL, GTK_STATE_FLAG_PRELIGHT,
                                  GTK_STATE_FLAG_ACTIVE, GTK_STATE_FLAG_INSENSITIVE};
  for (int i = 0; i < 4; i++)
  {
    gtk_widget_set_state_flags (button, states[i], TRUE);
    cairo_surface_t *surface = render_widget (button);
    int width = cairo_image_surface_get_width (surface);
    int height = cairo_image_surface_get_height (surface);
    guint32 *row = (guint32 *)(cairo_image_surface_get_data (surface) +
                              (height / 2) * cairo_image_surface_get_stride (surface));
    guint32 expected = (colors[i] >> 8) | 0xff000000;
    g_assert_cmphex (row[width / 2], ==, expected);
    cairo_surface_destroy (surface);
  }
  gtk_widget_destroy (window);
}

static void
chooser_test (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *chooser = ol_app_chooser_widget_new ();
  GError *error = NULL;
  GAppInfo *info = g_app_info_create_from_commandline ("true", "Test player", 0, &error);
  g_assert_no_error (error);
  GList *apps = g_list_append (NULL, info);
  ol_app_chooser_widget_set_app_list (OL_APP_CHOOSER_WIDGET (chooser), apps, 1);
  g_list_free (apps);
  g_object_unref (info);
  gtk_container_add (GTK_CONTAINER (window), chooser);
  gtk_widget_show_all (window);
  cairo_surface_t *surface = render_widget (chooser);
  cairo_surface_destroy (surface);
  g_assert_cmpuint (ol_app_chooser_widget_get_columns (OL_APP_CHOOSER_WIDGET (chooser)), ==, 1);
  gtk_widget_destroy (window);
}

static void
player_chooser_test (void)
{
  GtkWidget *window = ol_player_chooser_new (NULL);
  ol_player_chooser_set_info_by_state (OL_PLAYER_CHOOSER (window), OL_PLAYER_CHOOSER_STATE_NO_PLAYER);
  gtk_widget_show (window);
  cairo_surface_t *surface = render_widget (window);
  assert_has_pixels (surface);
  cairo_surface_destroy (surface);
  gtk_widget_destroy (window);
}

static void
cell_test (void)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *view = gtk_tree_view_new ();
  GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
  GtkTreeIter iter;
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter, 0, "Shortcut", -1);
  GtkCellRenderer *cell = ol_cell_renderer_button_new ();
  g_object_set (cell, "stock", GTK_STOCK_CLEAR, NULL);
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("Action", cell, "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
  gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (store));
  g_object_unref (store);
  gtk_container_add (GTK_CONTAINER (window), view);
  gtk_widget_show_all (window);
  cairo_surface_t *surface = render_widget (view);
  assert_has_pixels (surface);
  cairo_surface_destroy (surface);
  gtk_widget_destroy (window);
}

static void
builder_test (void)
{
  GtkBuilder *builder = gtk_builder_new ();
  GError *error = NULL;
  gtk_builder_add_from_file (builder, TEST_UI_FILE, &error);
  g_assert_no_error (error);
  const char *windows[] = {"aboutdialog", "optiondialog", "search-dialog", "downloaddialog"};
  for (guint i = 0; i < G_N_ELEMENTS (windows); i++)
  {
    GObject *object = gtk_builder_get_object (builder, windows[i]);
    g_assert_true (GTK_IS_WINDOW (object));
    gtk_widget_destroy (GTK_WIDGET (object));
  }
  g_object_unref (builder);
}

int
main (int argc, char **argv)
{
  gtk_test_init (&argc, &argv, NULL);
  g_test_add_func ("/gtk3/osd", osd_test);
  g_test_add_func ("/gtk3/scroll", scroll_test);
  g_test_add_func ("/gtk3/sprite-button", button_test);
  g_test_add_func ("/gtk3/app-chooser", chooser_test);
  g_test_add_func ("/gtk3/cell-renderer", cell_test);
  g_test_add_func ("/gtk3/player-chooser", player_chooser_test);
  g_test_add_func ("/gtk3/builder", builder_test);
  return g_test_run ();
}
