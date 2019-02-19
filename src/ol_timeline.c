/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2012  Tiger Soldier <tigersoldier@gmail.com>
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

#include "ol_timeline.h"
#include <stdlib.h>
#include <sys/time.h>
#include "ol_debug.h"

enum OlTimelineStatus {
  OL_TIMELINE_PLAYING,
  OL_TIMELINE_PAUSED,
  OL_TIMELINE_STOPPED
};

struct _OlTimeline
{
  int cached_time;
  struct timeval cached_time_begin;
  enum OlTimelineStatus status;
  int accuracy;
};

OlTimeline *
ol_timeline_new (void)
{
  OlTimeline *timeline = g_new (OlTimeline, 1);
  timeline->cached_time = 0;
  timeline->status = OL_TIMELINE_STOPPED;
  timeline->accuracy = 1000;
  return timeline;
}

void
ol_timeline_free (OlTimeline *timeline)
{
  g_free (timeline);
}

void
ol_timeline_play (OlTimeline *timeline)
{
  ol_assert (timeline != NULL);
  if (timeline->status == OL_TIMELINE_PLAYING)
    return;
  timeline->status = OL_TIMELINE_PLAYING;
  gettimeofday (&timeline->cached_time_begin, NULL);
}

void
ol_timeline_pause (OlTimeline *timeline)
{
  ol_assert (timeline != NULL);
  if (timeline->status == OL_TIMELINE_PAUSED)
    return;
  if (timeline->status == OL_TIMELINE_PLAYING)
    timeline->cached_time = ol_timeline_get_time (timeline);
  timeline->status = OL_TIMELINE_PAUSED;
}

void
ol_timeline_stop (OlTimeline *timeline)
{
  ol_assert (timeline != NULL);
  timeline->cached_time = 0;
  timeline->status = OL_TIMELINE_STOPPED;
}

void
ol_timeline_maybe_set_time (OlTimeline *timeline,
                            gint64 time_in_ms)
{
  ol_assert (timeline != NULL);
  gint64 current_time = ol_timeline_get_time (timeline);
  if (abs (current_time - time_in_ms) > timeline->accuracy)
  {
    ol_timeline_set_time (timeline, time_in_ms);
  }
}

void
ol_timeline_set_time (OlTimeline *timeline,
                      gint64 time_in_ms)
{
  ol_assert (timeline != NULL);
  if (timeline->status == OL_TIMELINE_STOPPED)
    return;
  timeline->cached_time = time_in_ms;
  gettimeofday (&timeline->cached_time_begin, NULL);
}

gint64
ol_timeline_get_time (OlTimeline *timeline)
{
  ol_assert_ret (timeline != NULL, 0);
  if (timeline->status == OL_TIMELINE_PLAYING)
  {
    struct timeval current_time;
    gettimeofday (&current_time, NULL);
    gint64 time = timeline->cached_time +
      (current_time.tv_sec - timeline->cached_time_begin.tv_sec) * 1000 +
      (current_time.tv_usec - timeline->cached_time_begin.tv_usec) / 1000;
    return time;
  }
  else
  {
    return timeline->cached_time;
  }
}

void
ol_timeline_set_accuracy (OlTimeline *timeline,
                          guint accuracy)
{
  ol_assert (timeline != NULL);
  timeline->accuracy = accuracy;
}

guint
ol_timeline_get_accuracy (OlTimeline *timeline)
{
  ol_assert_ret (timeline != NULL, 0);
  return timeline->accuracy;
}
