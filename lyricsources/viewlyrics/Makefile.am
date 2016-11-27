viewlyrics_PYTHON = viewlyrics.py

viewlyricsdir = $(pkglibdir)/lyricsources/viewlyrics


servicedir = $(datadir)/dbus-1/services
service_in_files = org.osdlyrics.LyricSourcePlugin.viewlyrics.service.in
service_DATA = $(service_in_files:.service.in=.service)

EXTRA_DIST = \
	$(service_in_files) \
	$(NULL)

$(service_DATA): $(service_in_files)
	@sed -e "s|\@pkglibdir\@|$(pkglibdir)|" -e "s|\@PYTHON\@|$(PYTHON)|" $< > $@

CLEANFILES = \
	org.osdlyrics.LyricSourcePlugin.viewlyrics.service \
	$(NULL)
