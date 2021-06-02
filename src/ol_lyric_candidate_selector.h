/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2012  Tiger Soldier <tigersoldier@gmail.com>
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
#ifndef _OL_LYRIC_CANDIDATE_SELECTOR_H_
#define _OL_LYRIC_CANDIDATE_SELECTOR_H_

#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtktreeselection.h>

#include "ol_metadata.h"
#include "ol_lyric_source.h"

typedef void (*OlLrcFetchUiDownloadFunc) (OlLyricSourceCandidate *candidate,
                                          const OlMetadata *metadata);

void ol_lyric_candidate_selector_show (GList *candidates,
                                       const OlMetadata *metadata,
                                       OlLrcFetchUiDownloadFunc download_func);

#endif /* _OL_LYRIC_CANDIDATE_SELECTOR_H_ */
