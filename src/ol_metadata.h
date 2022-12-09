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
#ifndef _OL_METADATA_H_
#define _OL_METADATA_H_
#include <stdlib.h>
#include <glib.h>

/**
 * defines a music's infomation structure
 */
typedef struct _OlMetadata OlMetadata;
struct _OlMetadata;

OlMetadata *ol_metadata_new (void);

/**
 * Create a metadata object from GVariant of MPRIS2 metadata dictionary
 *
 * @param variant
 *
 * @return
 */
OlMetadata *ol_metadata_new_from_variant (GVariant *variant);
void ol_metadata_free (OlMetadata *metadata);

/**
 * @brief Clears an OlMetadata
 * All fields will be freed if they are not empty
 *
 * @param metadata
 */
void ol_metadata_clear (OlMetadata *metadata);
void ol_metadata_copy (OlMetadata *dest, const OlMetadata *src);
OlMetadata *ol_metadata_dup (const OlMetadata *src);

/**
 * @brief Sets the value of music title
 *
 * @param metadata An OlMetadata
 * @param title The value of title. If not NULL, it will be copied
 *              inside the metadata
 */
void ol_metadata_set_title (OlMetadata *metadata,
                            const char *title);
const char *ol_metadata_get_title (const OlMetadata *metadata);

/**
 * @brief Sets the artist of music
 *
 * @param metadata An OlMetadata
 * @param artist The value of artist. If not NULL, it will be copied
 *               inside the metadata
 */
void ol_metadata_set_artist (OlMetadata *metadata,
                             const char *artist);
const char *ol_metadata_get_artist (const OlMetadata *metadata);

/**
 * @brief Sets the name of music album
 *
 * @param metadata An OlMetadata
 * @param album The name of album. If not NULL, it will be copied
 *              inside the metadata
 */
void ol_metadata_set_album (OlMetadata *metadata,
                            const char *album);
const char *ol_metadata_get_album (const OlMetadata *metadata);

void ol_metadata_set_track_number (OlMetadata *metadata,
                                   int track_number);
void ol_metadata_set_track_number_from_string (OlMetadata *metadata,
                                               const char *track_number);
int ol_metadata_get_track_number (const OlMetadata *metadata);

/**
 * @brief Sets the location of music file
 *
 * @param metadata An OlMetadata
 * @param uri The value of uri. If not NULL, it will be copied
 *            inside the metadata
 */
void ol_metadata_set_uri (OlMetadata *metadata,
                          const char *uri);
const char *ol_metadata_get_uri (const OlMetadata *metadata);

/**
 * @brief Sets the location of the album art
 *
 * The album art should be in file:// scheme.
 *
 * @param metadata The metadata
 * @param art_uri The uri if the album art.
 */
void ol_metadata_set_art (OlMetadata *metadata,
                          const char *art_uri);

/**
 * @brief Gets the location of the album art
 *
 * The album art is in URI format, which the local files will be in
 * file:// scheme
 *
 * @param metadata
 *
 * @return The uri of the album art, or NULL if not exists.
 */
const char *ol_metadata_get_art (const OlMetadata *metadata);

/**
 * Sets the duration of the metadata.
 *
 * @param metadata
 * @param duration The duration in millisecond.
 */
void ol_metadata_set_duration (OlMetadata *metadata,
                               guint64 duration);
/**
 * Gets the duration of the track, in millisecond
 *
 * @param metadata
 *
 * @return The duration of the track.
 */
guint64 ol_metadata_get_duration (const OlMetadata *metadata);

/**
 * @brief Sanitize the title and artist fields
 *
 * @param metadata A Metadata
 *
 * If the artist field is absent or looks invalid, check if the title field
 * actually contains multiple tags, and if so update the artist and title
 * fields in place. Supported formats (whitespace ignored):
 *
 *     [<track>.]<artist>SEP<title>
 *
 * where SEP can be either "-" or "--".
 */
void ol_metadata_sanitize_title_artist (OlMetadata *metadata);

/**
 * @brief Check whether two Metadatas are equal
 * Two Metadatas are equal if and only if all their fields are equal
 *
 * @param lhs An OlMetadata, or NULL
 * @param rhs An OlMetadata, or NULL
 *
 * @return If lhs is equal to rhs, return 1. Otherwise return 0
 */
int ol_metadata_equal (const OlMetadata *lhs,
                       const OlMetadata *rhs);


/**
 * @brief Converts a music info to a string
 * The returned buffer is NUL-terminated
 * @param metadata A Metadata
 * @param buffer Buffer of serialzed string, or NULL.
 *               If not NULL, the serialzed string is terminated with NUL.
 * @param count The size of the buffer.
 *
 * @return The length of the serialized string, regardless of the size of buffer.
 */
int ol_metadata_serialize (OlMetadata *metadata,
                           char *buffer,
                           size_t count);

/**
 * @brief Converts a string to an OlMetadata
 *
 * @param metadata A Metadata
 * @param data The serialized string from an OlMetadata
 *
 * @return 1 if succeeded, or 0 if failed
 */
int ol_metadata_deserialize (OlMetadata *metadata,
                             const char *data);

/**
 * Converts a metadata to a GVariant.
 *
 * This function is intended to do D-Bus communication.
 *
 * The conveted GVariant is a dict of type a{sv}. The content of the dict follows
 * the format of metadata defined in the Daemon's D-Bus specification.
 *
 * @param metadata
 *
 * @return A GVariant of a{sv}
 */
GVariant *ol_metadata_to_variant (OlMetadata *metadata);
#endif /* _OL_METADATA_H_ */
