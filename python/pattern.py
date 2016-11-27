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
# along with OSD Lyrics.  If not, see <http://www.gnu.org/licenses/>.
#

import urlparse
import urllib
import os.path
from errors import PatternException

def expand_file(pattern, metadata):
    """
    Expands the pattern to a file name according to the infomation of a music

    The following are supported place holder in the pattern:
    - %t: Title of the track. 'title' in metadata
    - %p: Performer (artist) of the music. 'artist' in metadata
    - %a: Album of the music. 'album' in metadata
    - %n: Track number of the music. 'tracknumber' in metadata
    - %f: Filename without extension of the music. 'location' in metadata.
    - %%: The `%' punctuation

    Arguments:
    - `pattern`: The pattern to expand.
    - `metadata`: A dict representing metadata. Useful keys are listed above.

    If the pattern cannot be expand, raise an PatternException. Otherwise
    return the expended pattern.

    >>> metadata = {'artist': 'Foo',
    ... 'title': 'Bar',
    ... 'tracknumber': '1',
    ... 'album': 'Album',
    ... 'location': 'file:///%E6%AD%8C%E6%9B%B2/%E7%9A%84/%E5%9C%B0%E5%9D%80.mp3'}
    >>> expand_file('%p - %t', metadata)
    'Foo - Bar'
    >>> expand_file('foobar', metadata)
    'foobar'
    >>> print expand_file('name is %f :)', metadata)
    name is 地址 :)
    >>> expand_file('%something else', metadata)
    '%something else'
    >>> expand_file('%%a - %%t', metadata)
    '%a - %t'
    >>> expand_file('%%%', metadata)
    '%%'
    >>> expand_file('%n - %a:%p,%t', metadata)
    '1 - Album:Foo,Bar'
    >>> expand_file('%t', {})
    Traceback (most recent call last):
        ...
    PatternException: 'title not in metadata'
    """
    keys = {'t': 'title',
            'p': 'artist',
            'a': 'album',
            'n': 'tracknum',
            }
    start = 0
    parts = []
    while start < len(pattern):
        end = pattern.find('%', start)
        if end > -1:
            parts.append(pattern[start:end])
            has_tag = False
            if end + 1 < len(pattern):
                tag = pattern[end + 1]
                if tag == '%':
                    has_tag = True
                    parts.append('%')
                elif tag == 'f':
                    location = metadata.location
                    if not location:
                        raise PatternException('Location not found in metadata')
                    uri = urlparse.urlparse(location)
                    if uri.scheme != '' and not uri.scheme in ['file']:
                        raise PatternException('Unsupported file scheme %s' % uri.scheme)
                    if uri.scheme == '':
                        path = uri.path
                    else:
                        path = urllib.url2pathname(uri.path)
                    basename = os.path.basename(path)
                    root, ext = os.path.splitext(basename)
                    has_tag = True
                    parts.append(root)
                elif tag in keys:
                    value = getattr(metadata, keys[tag])
                    if not value:
                        raise PatternException('%s not in metadata' % keys[tag])
                    has_tag = True
                    parts.append(value)
            if has_tag:
                start = end + 2
            else:
                start = end + 1
                parts.append('%')
        else:
            parts.append(pattern[start:])
            break
    return ''.join(parts)

def expand_path(pattern, metadata):
    """
    Expands the pattern to a directory path according to the infomation of a music

    The pattern can be one of the three forms:
    - begin with `/': the path is an absolute path and will not be expanded
    - begin with `~/': the path is an relative path and the `~' wiil be expanded to
      the absolute path of the user's home directory
    - `%': the path will be expanded to the directory of the music file according to
      its URI. ``location`` attribute is used in metadata

    Arguments:
    - `pattern`: The pattern to expand.
    - `metadata`: A dict representing metadata. Useful keys are listed above.

    If the pattern cannot be expand, raise an PatternException. Otherwise
    return the expended pattern.


    >>> expand_path('%', {'location': 'file:///tmp/a.lrc'})
    '/tmp'
    >>> expand_path('%foo', {'location': 'file:///tmp/a.lrc'})
    '%foo'
    >>> expand_path('/bar', {})
    '/bar'
    >>> expand_path('%', {'Title': 'hello'})
    Traceback (most recent call last):
        ...
    PatternException: 'Location not found in metadata'
    """
    if pattern == '%':
        location = metadata.location
        if not location:
            raise PatternException('Location not found in metadata')
        uri = urlparse.urlparse(location)
        if not uri.scheme in ['file']:
            raise PatternException('Unsupported file scheme %s' % uri.scheme)
        path = urllib.url2pathname(uri.path)
        return os.path.dirname(path)
    return os.path.expanduser(pattern)

if __name__ == '__main__':
    import doctest
    doctest.testmod()
