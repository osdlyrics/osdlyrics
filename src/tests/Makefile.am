check_PROGRAMS = \
        ol_osd_window_test \
        ol_player_test \
        ol_config_test \
        ol_color_test \
        ol_path_test \
        ol_utils_cmdline_test \
        ol_metadata_test \
        ol_utils_dbus_test \
        ol_utils_test \
        ol_gussian_blur_test \
        ol_app_info_test \
        ol_lyric_source_test \
        $(NULL)

AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
        -DDATADIR='"$(datadir)"' \
        -DICONDIR='"$(OL_ICONDIR)"' \
        -DGUIDIR='"$(OL_GUIDIR)"' \
        @GTK2_CFLAGS@ \
        @DBUS_GLIB_CFLAGS@ \
        @SQLITE_CFLAGS@ \
        -I$(top_srcdir)/src \
        -I$(top_srcdir)/src/tests \
        $(NULL)

AM_CFLAGS = \
        -Werror -Wall \
        $(NULL)

LDADD = \
        @GTK2_LIBS@ \
        @DBUS_GLIB_LIBS@ \
        @CURL_LIBS@ \
        @SQLITE_LIBS@ \
        -lm \
        $(NULL)

noinst_HEADERS = \
        ol_test_util.h

# Tests
ol_utils_cmdline_test_SOURCES = ol_utils_cmdline_test.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_utils_cmdline.c

ol_osd_window_test_SOURCES = ol_osd_window_test.c \
        $(top_srcdir)/src/ol_osd_window.c \
        $(top_srcdir)/src/ol_osd_render.c \
        $(top_srcdir)/src/ol_gussian_blur.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_color.c \
        $(top_srcdir)/src/ol_image_button.c \
        $(top_srcdir)/src/ol_utils.c

ol_player_test_SOURCES = ol_player_test.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_player.c \
        $(top_srcdir)/src/ol_timeline.c \
        $(top_srcdir)/src/ol_metadata.c \
        $(top_srcdir)/src/ol_utils.c \
        $(NULL)

ol_config_test_SOURCES = \
        $(top_srcdir)/src/ol_debug.c \
        ol_config_test.c \
        $(top_srcdir)/src/ol_config_proxy.c \
        $(top_srcdir)/src/ol_marshal.c

ol_color_test_SOURCES = \
        $(top_srcdir)/src/ol_debug.c \
        ol_color_test.c \
        $(top_srcdir)/src/ol_color.c

ol_path_test_SOURCES = \
         ol_path_test.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_path_pattern.c \
        $(top_srcdir)/src/ol_utils.c \
        $(top_srcdir)/src/ol_metadata.c

ol_metadata_test_SOURCES = \
        ol_metadata_test.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_metadata.c \
        $(top_srcdir)/src/ol_utils.c

ol_utils_dbus_test_SOURCES = \
        ol_utils_dbus_test.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_utils_dbus.c

ol_utils_test_SOURCES = \
        ol_utils_test.c \
        $(top_srcdir)/src/ol_utils.c \
        $(top_srcdir)/src/ol_debug.c

ol_gussian_blur_test_SOURCES = \
        ol_gussian_blur_test.c \
        $(top_srcdir)/src/ol_gussian_blur.c \
        $(top_srcdir)/src/ol_debug.c

ol_app_info_test_SOURCES = \
        ol_app_info_test.c \
        $(top_srcdir)/src/ol_app_info.c \
        $(top_srcdir)/src/ol_debug.c \
        $(NULL)

ol_lyric_source_test_SOURCES = \
        ol_lyric_source_test.c \
        $(top_srcdir)/src/ol_lyric_source.c \
        $(top_srcdir)/src/ol_debug.c \
        $(top_srcdir)/src/ol_marshal.c \
        $(top_srcdir)/src/ol_metadata.c \
        $(top_srcdir)/src/ol_utils.c \
        $(NULL)
