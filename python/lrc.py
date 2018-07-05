# -*- coding: utf-8 -*-
#
# Copyright (C) 2011  Tiger Soldier
#
# This file is part of OSD Lyrics.
#
# OSD Lyrics is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# OSD Lyrics is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OSD Lyrics.  If not, see <https://www.gnu.org/licenses/>.
#

from __future__ import print_function
from builtins import object
__all__ = (
    'AttrToken',
    'TimeToken',
    'StringToken',
    'tokenize',
    'parse_lrc',
)

import re

import dbus.types

LINE_PATTERN = re.compile(r'(\[[^\[]*?\])')
TIMESTAMP_PATTERN = re.compile(r'^\[(\d+(:\d+){0,2}(\.\d+)?)\]$')
ATTR_PATTERN = re.compile(r'^\[([\w\d]+):(.*)\]$')


class AttrToken(object):
    """
    Represents tags with the form of ``[key:value]``
    """

    def __init__(self, key, value):
        self.key = key
        self.value = value

    def __repr__(self):
        return '{%s: %s}' % (self.key, self.value)


class StringToken(object):
    """
    Represents a line of lyric text
    """

    def __init__(self, text):
        self.text = text

    def __repr__(self):
        return '"%s"\n' % self.text


class TimeToken(object):
    """
    Represents tags with the form of ``[h:m:s.ms]``

    The time attribute is the timestamp in milliseconds
    """

    def __init__(self, string):
        parts = string.split(':')
        parts.reverse()
        factor = 1000
        ms = int(float(parts[0]) * factor)
        for s in parts[1:]:
            factor = factor * 60
            ms = ms + factor * int(s)
        self.time = ms

    def __repr__(self):
        return '[%s]' % self.time


def tokenize(content):
    """ Split the content of LRC file into tokens

    Returns a list of tokens

    Arguments:
    - `content`: UTF8 string, the content to be tokenized
    """
    def parse_tag(tag):
        m = TIMESTAMP_PATTERN.match(tag)
        if m:
            return TimeToken(m.group(1))
        m = ATTR_PATTERN.match(tag)
        if m:
            return AttrToken(m.group(1), m.group(2))
        return None

    def tokenize_line(line):
        pos = 0
        tokens = []
        while pos < len(line) and line[pos] == '[':
            has_tag = False
            m = LINE_PATTERN.search(line, pos)
            if m and m.start() == pos:
                tag = m.group()
                token = parse_tag(tag)
                if token:
                    tokens.append(token)
                    has_tag = True
                    pos = m.end()
            if not has_tag:
                break
        tokens.append(StringToken(line[pos:]))
        return tokens

    lines = content.splitlines()
    tokens = []
    for line in lines:
        tokens.extend(tokenize_line(line))
    return tokens


def parse_lrc(content):
    """
    Parse an lrc file

    Arguments:
    - `content`: LRC file content encoded in UTF8

    Return values: attr, lyrics
    - `attr`: A dict represents attributes in LRC file
    - `lyrics`: A list of dict with 3 keys: id, timestamp and text.
      The list is sorted in ascending order by timestamp. Id increases from 0.
    """
    tokens = tokenize(content)
    attrs = {}
    lyrics = []
    timetags = []
    for token in tokens:
        if isinstance(token, AttrToken):
            attrs[token.key] = token.value
        elif isinstance(token, TimeToken):
            timetags.append(token.time)
        else:
            for timestamp in timetags:
                lyrics.append({'timestamp': dbus.types.Int64(timestamp),
                               'text': token.text})
            timetags = []
    lyrics.sort(key=lambda a: a['timestamp'])
    i = 0
    for lyric in lyrics:
        lyric['id'] = dbus.types.UInt32(i)
        i = i + 1
    return attrs, lyrics


def test():
    TEST_CASE1 = \
        """[ti:焔の扉~hearty edition][ar:FictionJunction YUUKA]
[al:焔の扉]
[02:45.59]その日まで
[52.78]
[03:48][35]焔の扉へ
[1:03:56.66][03:14.77]
おわり
"""

    def test_tokenizer():
        tokens = tokenize(TEST_CASE1)
        print(tokens)

    def test_parser():
        attr, lyrics = parse_lrc(TEST_CASE1)
        print(attr)
        for line in lyrics:
            print('%s: %s -> %s' % (line['id'], line['timestamp'], line['text']))

    test_tokenizer()
    test_parser()


if __name__ == '__main__':
    test()
