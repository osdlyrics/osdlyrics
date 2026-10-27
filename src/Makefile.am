bin_PROGRAMS = osdlyrics

#ol_player_test
#TEST = $(check_PROGRAMS)

AM_CPPFLAGS = \
	@GTK_CFLAGS@ \
	@DBUS_GLIB_CFLAGS@ \
	@NOTIFY_CFLAGS@ \
	@X11_CFLAGS@ \
	@APP_INDICATOR_CFLAGS@ \
	-DDATADIR='"$(datadir)"' \
	-DICONDIR='"$(OL_ICONDIR)"' \
	-DGUIDIR='"$(OL_GUIDIR)"'
LDADD = \
	@GTK_LIBS@ \
	@DBUS_GLIB_LIBS@ \
	@NOTIFY_LIBS@ \
	@X11_LIBS@ \
	@APP_INDICATOR_LIBS@ \
	-lm \
	$(NULL)

noinst_HEADERS = \
	gettext.h \
	ol_about.h \
	ol_app.h \
	ol_app_info.h \
	ol_app_chooser_widget.h \
	ol_cell_renderer_button.h \
	ol_color.h \
	ol_commands.h \
	ol_config_updater.h \
	ol_config_property.h \
	ol_config_proxy.h \
	ol_consts.h \
	ol_debug.h \
	ol_display_module.h \
	ol_eggaccelerators.h \
	ol_gui.h \
	ol_image_button.h \
	ol_intl.h \
	ol_keybinder.h \
	ol_keybinding_settings.h \
	ol_keybindings.h \
	ol_lyric_candidate_list.h \
	ol_lyric_candidate_selector.h \
	ol_lrc.h \
	ol_lyrics.h \
	ol_lyric_source.h \
	ol_lyric_source_list.h \
	ol_marshal.h \
	ol_md5.h \
	ol_menu.h \
	ol_metadata.h \
	ol_notify.h \
	ol_option.h \
	ol_osd_module.h \
	ol_osd_render.h \
	ol_osd_toolbar.h \
	ol_osd_window.h \
	ol_path_pattern.h \
	ol_player.h \
	ol_scroll_module.h \
	ol_scroll_window.h \
	ol_search_dialog.h \
	ol_stock.h \
	ol_timeline.h \
	ol_trayicon.h \
	ol_utils_cmdline.h \
	ol_utils_dbus.h \
	ol_utils_dcop.h \
	ol_utils_network.h \
	ol_gussian_blur.h \
	ol_player_chooser.h \
	ol_utils.h

osdlyrics_SOURCES = \
	ol_app_info.c \
	ol_app_chooser_widget.c \
	ol_debug.c \
	ol_main.c \
	ol_config_updater.c \
	ol_config_proxy.c \
	ol_gui.c \
	ol_about.c \
	ol_option.c \
	ol_display_module.c \
	ol_osd_window.c \
	ol_osd_toolbar.c \
	ol_osd_render.c \
	ol_osd_module.c \
	ol_scroll_module.c \
	ol_scroll_window.c \
	ol_image_button.c \
	ol_player.c \
	ol_md5.c \
	ol_utils.c \
	ol_utils_dbus.c \
	ol_utils_dcop.c \
	ol_utils_cmdline.c \
	ol_utils_network.c \
	ol_lrc.c \
	ol_lyrics.c \
	ol_search_dialog.c \
	ol_trayicon.c \
	ol_timeline.c \
	ol_menu.c \
	ol_lyric_candidate_selector.c \
	ol_lyric_candidate_list.c \
	ol_lyric_source.c \
	ol_lyric_source_list.c \
	ol_metadata.c \
	ol_color.c \
	ol_eggaccelerators.c \
	ol_keybinder.c \
	ol_keybindings.c \
	ol_commands.c \
	ol_path_pattern.c \
	ol_cell_renderer_button.c \
	ol_marshal.c \
	ol_stock.c \
	ol_notify.c \
	ol_player_chooser.c \
	ol_gussian_blur.c

#osd_lyrics_CXXFLAGS = @GTK_CFLAGS@


# Librarys

SUBDIRS = tests

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@
