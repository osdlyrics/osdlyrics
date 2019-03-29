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
#ifndef _OL_TIMELINE_H_
#define _OL_TIMELINE_H_

#include <glib.h>

typedef struct _OlTimeline OlTimeline;

/** 
 * Create a new timeline.
 * 
 * @return The new timeline. Its status is stopped, time is 0, and accuracy is
 *         1000 milliseconds.
 */
OlTimeline *ol_timeline_new (void);

/** 
 * Free a timeline
 * 
 * @param timeline The timeline to be freed.
 */
void ol_timeline_free (OlTimeline *timeline);

/** 
 * Change status of a timeline to playing.
 *
 * The time of the timeline will keep increasing.
 * @param timeline A timeline.
 */
void ol_timeline_play (OlTimeline *timeline);

/** 
 * Change status of a timeline to paused.
 *
 * The time of the timeline will not be changed.
 * @param timeline A timeline.
 */
void ol_timeline_pause (OlTimeline *timeline);

/** 
 * Change status of a timeline to stopped.
 *
 * The time of the timeline will be reset to 0.
 * @param timeline A timeline.
 */
void ol_timeline_stop (OlTimeline *timeline);

/** 
 * Set time of a timeline if neccessary.
 *
 * If the difference between time of the timeline and the specified time is greater
 * than accuracy of the timeline, the time will be set. Otherwise the time will not
 * change.
 *
 * It is suggestted to use this function if the precesion of time_in_ms is greater
 * than milliseconds.
 * 
 * @param timeline A timeline.
 * @param time_in_ms The time to set, in milliseconds.
 */
void ol_timeline_maybe_set_time (OlTimeline *timeline,
                                 gint64 time_in_ms);

/** 
 * Set time of a timeline.
 *
 * @param timeline A timeline.
 * @param time_in_ms The time to set, in milliseconds.
 */
void ol_timeline_set_time (OlTimeline *timeline,
                           gint64 time_in_ms);

/** 
 * Get current time of a timeline.
 * 
 * @param timeline A timeline.
 * 
 * @return 
 */
gint64 ol_timeline_get_time (OlTimeline *timeline);

/** 
 * Set accuracy of a timeline.
 *
 * Accuracy only affects ol_timeline_maybe_set_time().
 * @param timeline 
 * @param accuracy 
 */
void ol_timeline_set_accuracy (OlTimeline *timeline,
                               guint accuracy);

/** 
 * Get accuracy of a timeline.
 * 
 * @param timeline A timeline.
 * 
 * @return 
 */
guint ol_timeline_get_accuracy (OlTimeline *timeline);

#endif /* _OL_TIMELINE_H_ */
