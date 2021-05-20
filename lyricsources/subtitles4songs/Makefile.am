subtitles4songs_PYTHON = subtitles4songs.py

subtitles4songsdir = $(pkglibdir)/lyricsources/subtitles4songs


servicedir = $(datadir)/dbus-1/services
service_in_files = org.osdlyrics.LyricSourcePlugin.subtitles4songs.service.in
service_DATA = $(service_in_files:.service.in=.service)

EXTRA_DIST = \
	$(service_in_files) \
	$(NULL)

$(service_DATA): $(service_in_files)
	@sed -e "s|\@pkglibdir\@|$(pkglibdir)|" -e "s|\@PYTHON\@|$(PYTHON)|" $< > $@

CLEANFILES = \
	org.osdlyrics.LyricSourcePlugin.subtitles4songs.service \
	$(NULL)
