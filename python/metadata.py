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

import logging
import re

import dbus

import utils

class Metadata(object):
    """
    Metadata of a track

    This class helps to deal with different metadata formats defined by MPRIS1,
    MPRIS2 and OSD Lyrics. It is recommended to parse a metadata dict from D-Bus
    with `Metadata.from_dict()`.

    Metadata provides following properties: `title`, `artist`, `album`, `location`,
    `arturl`, `length`, and `tracknum`, where `length` and `tracknum` are integers,
    the others are strings.
    """

    # Possible MPRIS metadata keys, taken from
    # http://xmms2.org/wiki/MPRIS_Metadata#MPRIS_v1.0_Metadata_guidelines"""
    MPRIS1_KEYS = set(['genre', 'comment', 'rating', 'year', 'date', 'asin',
                       'puid fingerprint', 'mb track id', 'mb artist id',
                       'mb artist sort name', 'mb album id', 'mb release date',
                       'mb album artist', 'mb album artist id',
                       'mb album artist sort name', 'audio-bitrate',
                       'audio-samplerate', 'video-bitrate'])

    # Possible MPRIS2 metadata keys, taken from
    # http://www.freedesktop.org/wiki/Specifications/mpris-spec/metadata
    MPRIS2_KEYS = set(['xesam:albumArtist', 'xesam:asText', 'xesam:audioBPM',
                       'xesam:autoRating', 'xesam:comment', 'xesam:composer',
                       'xesam:contentCreated', 'xesam:discNumber', 'xesam:firstUsed',
                       'xesam:genre', 'xesam:lastUsed', 'xesam:lyricist',
                       'xesam:useCount', 'xesam:userRating'])

    def __init__(self,
                 title=None,
                 artist=None,
                 album=None,
                 arturl=None,
                 tracknum=-1,
                 location=None,
                 length=-1,
                 extra={}):
        """
        Create a new Metadata instance.

        Arguments:
        - `title`: (string) The title of the track
        - `artist`: (string) The artist of the track
        - `album`: (string) The name of album that the track is in
        - `arturl`: (string) The URI of the picture of the cover of the album
        - `tracknum`: (int) The number of the track
        - `location`: (string) The URI of the file
        - `length`: (int) The duration of the track
        - `extra`: (dict) A dict that is intend to store additional properties
                   provided by MPRIS1 or MPRIS2 DBus dicts. The MPRIS1-related
                   values will be set in the dict returned by `to_mpris1`. The
                   MPRIS2-related values are treated in a similar way.
        """
        self.title = title
        self.artist = artist
        self.album = album
        self.arturl = arturl
        self.tracknum = tracknum
        self.location = location
        self.length = length
        self._extra = extra

    def to_mpris1(self):
        """
        Converts the metadata to mpris1 dict
        """
        ret = dbus.Dictionary(signature='sv')
        for k in ['title', 'artist', 'album', 'arturl', 'location']:
            if getattr(self, k) is not None:
                ret[k] = dbus.String(utils.ensure_unicode(getattr(self, k)))
        if self.tracknum >= 0:
            ret['tracknumber'] = dbus.String(self.tracknum)
        if self.length >= 0:
            ret['time'] = dbus.UInt32(self.length / 1000)
            ret['mtime'] = dbus.UInt32(self.length)
        for k, v in self._extra.items():
            if k in Metadata.MPRIS1_KEYS and k not in ret:
                ret[k] = v
        return ret

    def to_mpris2(self):
        """
        Converts the metadata to mpris2 dict

        >>> mt = Metadata(title='Title', artist='Artist1, Artist2,Artist3',
        ...               album='Album', arturl='file:///art/url',
        ...               location='file:///path/to/file', length=123,
        ...               tracknum=456,
        ...               extra={ 'title': 'Fake Title',
        ...                       'xesam:album': 'Fake Album',
        ...                       'xesam:useCount': 780,
        ...                       'xesam:userRating': 1.0,
        ...                       'custom value': 'yoooooo',
        ...                       })
        >>> dict = mt.to_mpris2()
        >>> print dict['xesam:title']
        Title
        >>> print dict['xesam:artist']
        [dbus.String(u'Artist1'), dbus.String(u'Artist2'), dbus.String(u'Artist3')]
        >>> print dict['xesam:url']
        file:///path/to/file
        >>> print dict['mpris:artUrl']
        file:///art/url
        >>> print dict['mpris:length']
        123
        >>> print dict['xesam:trackNumber']
        456
        >>> print dict['xesam:userRating']
        1.0
        >>> 'custom value' in dict
        False
        >>> mt2 = Metadata.from_dict(dict)
        >>> print mt2.title
        Title
        >>> print mt2.artist
        Artist1, Artist2, Artist3
        >>> print mt2.album
        Album
        >>> print mt2.location
        file:///path/to/file
        """
        ret = dbus.Dictionary(signature='sv')
        mpris2map = { 'title': 'xesam:title',
                      'album': 'xesam:album',
                      'arturl': 'mpris:artUrl',
                      'location': 'xesam:url',
                      }
        for k in ['title', 'album', 'arturl', 'location']:
            if getattr(self, k) is not None:
                ret[mpris2map[k]] = dbus.String(utils.ensure_unicode(getattr(self, k)))
        if self.artist is not None:
            ret['xesam:artist'] = [dbus.String(v.strip()) for v in self.artist.split(',')]
        if self.length >= 0:
            ret['mpris:length'] = dbus.Int64(self.length)
        if self.tracknum >= 0:
            ret['xesam:trackNumber'] = dbus.Int32(self.tracknum)
        for k, v in self._extra.items():
            if k in Metadata.MPRIS2_KEYS and k not in ret:
                ret[k] = v
        return ret

    @staticmethod
    def from_mpris2(mpris2_dict):
        """
        Create a Metadata object from mpris2 metadata dict
        """
        string_dict = {'title': 'xesam:title',
                       'album': 'xesam:album',
                       'arturl': 'mpris:artUrl',
                       'location': 'xesam:url',
                       }
        string_list_dict = {'artist': 'xesam:artist'}
        kargs = {}
        for k, v in string_dict.items():
            if v in mpris2_dict:
                kargs[k] = mpris2_dict[v]
        for k, v in string_list_dict.items():
            if v in mpris2_dict:
                kargs[k] = ', '.join(mpris2_dict[v])
        if 'xesam:trackNumber' in mpris2_dict:
            kargs['tracknum'] = int(mpris2_dict['xesam:trackNumber'])
        if 'mpris:length' in mpris2_dict:
            kargs['length'] = int(mpris2_dict['mpris:length'])
        ret = Metadata(**kargs)
        ret._extra = mpris2_dict
        return ret

    @staticmethod
    def from_dict(dbusdict):
        """
        Create a Metadata object from a D-Bus dict object.

        The D-Bus dict object can be MPRIS1 metadata or MPRIS2 metadata format. If
        the dict both compatable with MPRIS1 and MPRIS2, MPRIS1 will be used.

        >>> title = 'Title'
        >>> artist = 'Artist'
        >>> arturl = 'file:///art/url'
        >>> location = 'file///location'
        >>> tracknumber = 42
        >>> md1 = Metadata.from_dict({'title': title,
        ...                           'artist': artist,
        ...                           'arturl': arturl,
        ...                           'location': location,
        ...                           'tracknumber': str(tracknumber) + '/2'})
        >>> md1.title == title
        True
        >>> md1.artist == artist
        True
        >>> md1.arturl == arturl
        True
        >>> md1.location == location
        True
        >>> md1.tracknum == tracknumber
        True
        >>> md2 = Metadata.from_dict({'xesam:title': title,
        ...                           'xesam:artist': [artist],
        ...                           'mpris:artUrl': arturl,
        ...                           'xesam:url': location,
        ...                           'xesam:trackNumber': tracknumber})
        >>> md2.title == title
        True
        >>> md2.artist == artist
        True
        >>> md2.arturl == arturl
        True
        >>> md2.location == location
        True
        >>> md2.tracknum == tracknumber
        True
        >>> md3 = Metadata.from_dict({'title': title,
        ...                           'artist': artist,
        ...                           'arturl': arturl,
        ...                           'location': location,
        ...                           'tracknumber': str(tracknumber) + '/2',
        ...                           'xesam:title': title + '1',
        ...                           'xesam:artist': [artist + '1', '1'],
        ...                           'mpris:artUrl': arturl + '1',
        ...                           'xesam:url': location + '1',
        ...                           'xesam:trackNumber': tracknumber + 1})
        >>> md3.title == title
        True
        >>> md3.artist == artist
        True
        >>> md3.arturl == arturl
        True
        >>> md3.location == location
        True
        >>> md3.tracknum == tracknumber
        True
        >>> timedict = {'time': 10, 'mtime': 20, 'mpris:length': 3000}
        >>> Metadata.from_dict(timedict).length
        20
        >>> del timedict['mtime']
        >>> Metadata.from_dict(timedict).length
        3
        >>> del timedict['mpris:length']
        >>> Metadata.from_dict(timedict).length
        10000
        """
        string_dict = {'title': ['title', 'xesam:title'],
                       'album': ['album', 'xesam:album'],
                       'arturl': ['arturl', 'mpris:artUrl'],
                       'artist': ['artist'],
                       'location': ['location', 'xesam:url'],
                       }
        string_list_dict = {'artist': 'xesam:artist',
                            }
        kargs = {}
        for k, v in string_dict.items():
            for dict_key in v:
                if dict_key in dbusdict:
                    kargs[k] = dbusdict[dict_key]
                    break;
        # artist
        for k, v in string_list_dict.items():
            if k not in kargs and v in dbusdict:
                kargs[k] = ', '.join(dbusdict[v])
        # tracknumber
        if 'tracknumber' in dbusdict:
            tracknumber = dbusdict['tracknumber']
            if isinstance(tracknumber, int):
                # the specification requires tracknumber be a string. However,
                # tracknumber in audacious is int32
                kargs['tracknum'] = tracknumber
            else:
                if not re.match(r'\d+(/\d+)?', tracknumber):
                    logging.warning('Malfromed tracknumber: %s' % tracknumber)
                else:
                    kargs['tracknum'] = int(dbusdict['tracknumber'].split('/')[0])
        if 'tracknum' not in kargs and 'xesam:trackNumber' in dbusdict:
            kargs['tracknum'] = int(dbusdict['xesam:trackNumber'])

        # length
        if 'mtime' in dbusdict:
            kargs['length'] = dbusdict['mtime']
        elif 'mpris:length' in dbusdict:
            kargs['length'] = int(dbusdict['mpris:length'])
        elif 'time' in dbusdict:
            kargs['length'] = dbusdict['time'] * 1000
        ret = Metadata(**kargs)
        ret._extra = dbusdict
        return ret

    def __str__(self):
        attrs = ['title', 'artist', 'album', 'location', 'length']
        attr_value = ['  %s: %s' % (key, getattr(self, key)) for key in attrs]
        return 'metadata:\n' + '\n'.join(attr_value)

if __name__ == '__main__':
    import doctest
    doctest.testmod()
