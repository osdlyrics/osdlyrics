/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldier@gmail.com>
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
/**
 * @file   ol_keybinding.h
 * @author Tiger Soldier <tigersoldi@gmail.com>
 * @date   Sun Aug 16 15:58:56 2009
 *
 * @brief  Global Keybinding settings
 */
#ifndef _OL_KEYBINDING_H_
#define _OL_KEYBINDING_H_

#include <gtk/gtkaccelmap.h>
#include <gtk/gtkaccelgroup.h>

void ol_keybinding_init ();
GtkAccelGroup* ol_keybinding_get_accel_group ();

#endif /* _OL_KEYBINDING_H_ */
