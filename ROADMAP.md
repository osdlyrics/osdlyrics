# OSD Lyrics roadmap

## 0.5 release requirements (and further planning)

I envision 0.5 release as a rather stable one, wrapping up the work put into
it by Tiger Soldier et al. This release should not be focused on more features
because there hasn't been any in a long time, and the program is in quite a
good shape considering it was almost abandoned until recently. 0.5 release
should serve as a stabilization point before further feature work.

The following 0.5.x series should focus on further stabilization but should
also make the project take up some fundamental features, such as support for
the most important lyrics sites. (By the way, unsynchronized text could just
scroll in the windowed mode, even without timestamps it can be useful, see
lrcShow-X.)

0.6 release should follow after a few 0.5.x development releases, accumulating
just a few smaller improvements. It should be rock-solid and only receive
bugfixes. See below for 0.6 release criteria.

### Iron out some known bugs

* OSD panel is not click-through when the panel is initialized in 'Locked'
  mode. Switching the mode back and forth fixes that. But the problem comes
  back when the panel is supposedly destroyed and then recreated by switching
  to windowed mode and back to OSD mode.
  The following GTK errors (from deep inside the library) appear when the panel
  is clicked when in this troubled state:

When a mouse button is pressed:

```
(OSD Lyrics:8291): Gdk-CRITICAL **: IA__gdk_window_get_events: assertion 'GDK_IS_WINDOW (window)' failed
(OSD Lyrics:8291): GLib-GObject-CRITICAL **: g_object_ref: assertion 'G_IS_OBJECT (object)' failed
```

When the button is released:

```
(OSD Lyrics:8291): GLib-GObject-CRITICAL **: g_object_unref: assertion 'G_IS_OBJECT (object)' failed
```

* Some lyrics (with Chinese characters, I think) cause a crash in Cairo:

  `osdlyrics: cairo-scaled-font.c:459: _cairo_scaled_glyph_page_destroy:`
  `Assertion !scaled_font->cache_frozen failed.`

### Communicate with players flawlessly

* MPRIS 1.0 works quite well but timing becomes inaccurate when a player seeks
  to anywhere but the beginning of a track.

* I couldn't get MPRIS 2.x to work well with VLC or Audacious.

* MPD proxy needs to be tested (at all).

* What is this HTTP controller supposed to do?

### Have all lyrics sources working

* Or at least make it possible to disable/prioritize them (disabling seems to
  be ignored now).

* ViewLyrics works very well but certain parsing errors or network errors
  should not cause serious exceptions, that is, they should result only in
  brief log messages, not whole tracebacks.

* LRC123 site is down so the plugin should be disabled for now. Not removed, if
  there is a chance of the site going back.

* Xiami should be tested.

### Nice things to have

* Extended LRC format is not handled at all currently. <mm:ss.ms> timestamps
  should now be either plain removed, or tokenized but not shown. An example:

  `[00:33.60]Étaient <00:33.79>sur <00:33.93>terre.`

* Some more, prominent lyrics sources?

* Dealing with imperfect metadata with some basic heuristics: splitting song
  title and its artist mixed up in a single string; figuring this from the file
  name.

* Prioritizing lyrics by user rating (ViewLyrics have scores, don't they?).

## 0.6 release requirements (to be fulfilled during 0.5.x development series)

* Switch to Python 3.

* Support scrolling plain text in windowed mode (not in OSD mode of course).

* Enhanced LRC format if there are more than just a handful of lyrics using it.

* Better documentation.

## Future releases

After 0.6 is released, it enters maintenance mode and receives only bugfixes in
0.6.x series. 0.7 development starts, focusing on exciting features, such as
any drawn from a wish list:

* Prefetch lyrics for the next track by peeking in a playlist.

* Smoother scrolling, improved blur.

* More modes: multiple scrolling lines in OSD?

* Music played on web sites could use lyrics too, so cooperate with web
  browsers -- try to find or implement third-party MPRIS 2.x proxies for them.
  The same applies to streams (radio).

* LRC editor, see e.g. lrcShow-X, or LRC editing mode. We already can change
  the global offset by scrolling over the tray icon thanks to Corax26. So how
  about changing the offset of the current line in lyrics editing mode (either
  by the scrolling button of even using the mouse to shift the line
  horizontally)?
