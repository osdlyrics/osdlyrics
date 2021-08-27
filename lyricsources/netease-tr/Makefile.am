netease_tr_PYTHON = netease_tr.py

netease_trdir = $(pkglibdir)/lyricsources/netease_tr


servicedir = $(datadir)/dbus-1/services
service_in_files = org.osdlyrics.LyricSourcePlugin.netease_tr.service.in
service_DATA = $(service_in_files:.service.in=.service)

EXTRA_DIST = \
	$(service_in_files) \
	$(NULL)

$(service_DATA): $(service_in_files)
	@sed -e "s|\@pkglibdir\@|$(pkglibdir)|" -e "s|\@PYTHON\@|$(PYTHON)|" $< > $@

CLEANFILES = \
	org.osdlyrics.LyricSourcePlugin.netease_tr.service \
	$(NULL)
