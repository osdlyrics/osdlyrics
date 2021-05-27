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
#ifndef __OL_UTILS_H__
#define __OL_UTILS_H__
#include <unistd.h>
#include <glib.h>
#include <gio/gio.h>

#define ol_get_array_len(arr) (sizeof (arr) / sizeof (arr[0]))

const gchar* ol_get_string_from_hash_table (GHashTable *hash_table, const gchar *name);
gchar** ol_get_str_list_from_hash_table (GHashTable *hash_table, const gchar *name);
gint ol_get_int_from_hash_table (GHashTable *hash_table, const gchar *name);
guint ol_get_uint_from_hash_table (GHashTable *hash_table, const gchar *name);
gint64 ol_get_int64_from_hash_table (GHashTable *hash_table, const gchar *name);

/**
 * @brief Checks if a string is empty
 * A string is empty if it is NULL or contains only white spaces
 * @param str The string to be checked
 *
 * @return If the string is empty, returns true
 */
gboolean ol_is_string_empty (const char *str);

/**
 * @brief allocate memory for pathname, from APUE
 *
 * @return pointer to this memory if success, or NULL. Should be free with free()
 */
char* ol_path_alloc(void);

/**
 * @brief comparing str1 with str2 case insensitive
 *
 * @param str1
 * @param str2
 * @param count The number of characters to be compared with, or -1 to compare
 *              the whole strings
 *
 * @return the same with the function: strcmp in <string.h>
 */
int ol_stricmp(const char *str1, const char *str2, const ssize_t count);

/**
 * @brief Copy the source string to the destination string
 * If the length to be coyped is larger than that of the destination string,
 * the copying will fail, and returns NULL.
 * @param dest Destination string
 * @param dest_len The length of destination string, including '\0'
 * @param src Source string
 * @param src_len The length of source string to be copyed
 *
 * @return The end of the destination if suceeded, or NULL if failed.
 */
char* ol_memcpy (char *dest,
                   size_t dest_len,
                   const char *src,
                   size_t src_len);

/**
 * @Calculates the largest common substring for two strings
 *
 * @param str1
 * @param str2
 *
 * @return the length of the largest common substring
 */
size_t ol_lcs (const char *str1, const char *str2);

/**
 * @Checks whether two strings are equale
 * Strings are different if one of them is NULL but another not,
 * or their content are different
 *
 * @param str1 The first string or NULL
 * @param str2 The second string or NULL
 *
 * @return TRUE if they are equal
 */
int ol_streq (const char *str1, const char *str2);

/**
 * @Copy the content of a string pointer to another
 *
 * @param dest The target string pointer. If it doesn't point to NULL, \
 * it will be free with g_free
 * @param src The source string. If it is not NULL, \
 * it will be copyed with g_strdup, the dupped string will be assigned to  \
 * dest. Otherwise the dest will be NULL after copied.
 *
 * @return The string that dest points to after copying
 */
char *ol_strptrcpy (char **dest, const char *src);

/**
 * @brief Find the first '\n', change it into '\0' and return the pointer to \
 *        the next line
 *
 * @param str the string to be splited, cannot be NULL
 *
 * @return The pointer to the next line. If no '\n' exists, return NULL
 */
char *ol_split_a_line (char *str);

/**
 * @brief Fill the heading and tailing white spaces of str with '\0', \
 *        and returns the pointer to the first non-space character. \
 *        If the string is empty, simply return NULL
 *
 * @param str The string to be trimed. If it is NULL, returns NULL.
 *
 * @return The pointer to the first non-space character, \
 *         or NULL if str is empty or NULL.
 */
char *ol_trim_string (char *str);

/**
 * @brief Check whether a file exists and is a regular file.
 *
 * @param filename The path and fullname of the file
 *
 * @return TRUE if the file in the path exists and is a regular file
 */
gboolean ol_path_is_file (const char *filename);

/**
 * @brief Gets the length of a file
 *
 * @param filename The full path of the file
 *
 * @return The length of a file, or negative if error occurs
 */
ssize_t ol_file_len (const char *filename);

/**
 * Converts a string to hex representation.
 *
 * @param data The string to encode
 * @param len The length to encode, or -1 if the string is NUL-terminated
 *
 * @return The encoded string, should be freed with g_free.
 */
char* ol_encode_hex (const char *data, ssize_t len);

/**
 * Split a path into two parts (root, ext), the ext is the extension of a file
 * name, which begins with a period and contains at most period. root + ext = path.
 *
 * If the filename begins with a period and has no extension, the filename will not
 * be stored in the ext part.
 *
 * @param path The path to be splited.
 * @param root The path without ext. Should be freed with g_free.
 * @param ext The extension of filename, or NULL if no extension found.
 *            Should be freed with g_free.
 */
void ol_path_splitext (const char *path, char **root, char **ext);

/**
 * Compares two GAppInfo according to its name.
 *
 * The comparison is case-insensitive
 *
 * @param a
 * @param b
 *
 * @return 0 if the name of a equals to be.
 *         Negative if a is less than b.
 *         Otherwise a is greater than b.
 */
gint ol_app_info_cmp (GAppInfo *a, GAppInfo *b);

/**
 * Launch a commandline from GAppInfo
 *
 * @param cmdline The commandline to launch. %f and %U will be ignored
 *
 * @return TRUE on successful launch, FALSE otherwise.
 */
gboolean ol_launch_app (const char *cmdline);

/**
 * Pass names of all files and directories in the specified directory to given
 * function.
 *
 * @param dir The path of the directroy
 * @param recursive Whether to visit subdirectories in the directory
 * @param traverse_func The function to receive the file name. traverse_func will be
 *                      called on each file once. If it returns FALSE, traversal will
 *                      be ended.
 * @param userdata
 *
 * @return TRUE if the traversal is no interrupted because traverse_func returns
 *         FALSE. The return value is intend to be used in recursive traversal.
 */
gboolean ol_traverse_dir (const char *dir,
                          gboolean recursive,
                          gboolean (*traverse_func) (const char *path,
                                                     const char *filename,
                                                     gpointer userdata),
                          gpointer userdata);
#endif // __OL_UTILS_H__
