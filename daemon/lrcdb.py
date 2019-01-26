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
from __future__ import unicode_literals
from future import standard_library
standard_library.install_aliases()
from builtins import object
import logging
import sqlite3
import os.path
import osdlyrics.utils
from osdlyrics.consts import METADATA_URI, METADATA_TITLE, METADATA_ALBUM, \
    METADATA_ARTIST, METADATA_TRACKNUM

__all__ = (
    'LrcDb',
    )


def query_param_from_metadata(metadata):
    """
    Generate query dict from metadata
    """
    param = {
        METADATA_TITLE: metadata.title if metadata.title is not None else '',
        METADATA_ARTIST: metadata.artist if metadata.artist is not None else '',
        METADATA_ALBUM: metadata.album if metadata.album is not None else '',
        }
    try:
        tracknum = int(metadata.tracknum)
        if tracknum < 0:
            tracknum = 0
    except:
        tracknum = 0
    param[METADATA_TRACKNUM] = tracknum
    return param

class LrcDb(object):
    """ Database to store location of LRC files that have been manually assigned
    """
    
    TABLE_NAME = 'lyrics'

    METADATA_LIST = [METADATA_TITLE, METADATA_ARTIST, METADATA_ALBUM, METADATA_TRACKNUM]

    CREATE_TABLE = """
CREATE TABLE IF NOT EXISTS {0} (
  id INTEGER PRIMARY KEY AUTOINCREMENT, 
  {1} TEXT, {2} TEXT, {3} TEXT, {4} INTEGER,
  uri TEXT UNIQUE ON CONFLICT REPLACE,
  lrcpath TEXT
)
""".format(TABLE_NAME, *METADATA_LIST)

    ASSIGN_LYRIC = """
INSERT OR REPLACE INTO {0}
  ({1}, {2}, {3}, {4}, uri, lrcpath)
  VALUES (?, ?, ?, ?, ?, ?)
""" .format(TABLE_NAME, *METADATA_LIST)

    UPDATE_LYRIC = """
UPDATE {0}
  SET lrcpath=?
  WHERE uri=?
""".format(TABLE_NAME)

    DELETE_LYRIC = 'DELETE FROM {0} WHERE '.format(TABLE_NAME)
    
    FIND_LYRIC = 'SELECT lrcpath FROM {0} WHERE '.format(TABLE_NAME)

    QUERY_LOCATION = 'uri = ?'

    QUERY_INFO = ' AND '.join('{0}=:{0}'.format(m) for m in METADATA_LIST)

    def __init__(self, dbfile=None):
        """
        
        Arguments:
        - `dbfile`: The sqlite db to open
        """
        if dbfile is None:
            dbfile = osdlyrics.utils.get_config_path('lrc.db')
        self._dbfile = dbfile
        osdlyrics.utils.ensure_path(dbfile)
        self._conn = sqlite3.connect(os.path.expanduser(dbfile))
        self._create_table()

    def _create_table(self):
        """ Ensures the table structure of new open dbs
        """
        c = self._conn.cursor()
        c.execute(LrcDb.CREATE_TABLE)
        self._conn.commit()
        c.close()

    def assign(self, metadata, uri):
        # type: (osdlyrics.metadata.Metadata, Text) -> None
        """ Assigns a uri of lyrics to tracks represented by metadata
        """
        c = self._conn.cursor()
        if metadata.location:
            location = metadata.location
        else:
            location = ''
        if self._find_by_location(metadata):
            logging.debug('Assign lyric file %s to track of location %s' % (uri, location))
            c.execute(LrcDb.UPDATE_LYRIC, (uri, location,))
        else:
            title = metadata.title if metadata.title is not None else ''
            artist = metadata.artist if metadata.artist is not None else ''
            album = metadata.album if metadata.album is not None else ''
            try:
                tracknum = int(metadata.tracknum)
                if tracknum < 0:
                    tracknum = 0
            except:
                tracknum = 0
            logging.debug('Assign lyrics file %s to track %s. %s - %s in album %s @ %s' % (uri, tracknum, artist, title, album, location))
            c.execute(LrcDb.ASSIGN_LYRIC, (title, artist, album, tracknum, location, uri))
        self._conn.commit()
        c.close()

    def delete(self, metadata):
        """ Deletes lyrics association(s) for given metadata

        Deletes all lyrics associations that would be found by find(self, metadata)
        """
        c = self._conn.cursor()

        if metadata.location:
            c.execute(LrcDb.DELETE_LYRIC + LrcDb.QUERY_LOCATION, (metadata.location,))

        c.execute(LrcDb.DELETE_LYRIC + LrcDb.QUERY_INFO, query_param_from_metadata(metadata))

        self._conn.commit()
        c.close()

    def find(self, metadata):
        """ Finds the location of LRC files for given metadata

        To find the location of lyrics, firstly find whether there is a record matched
        with the ``location`` attribute in metadata. If not found or ``location`` is
        not specified, try to find with respect to ``title``, ``artist``, ``album``
        and ``tracknum``
        
        If found, return the uri of the LRC file. Otherwise return None. Note that
        this method may return an empty string, so use ``is None`` to figure out
        whether an uri is found
        """
        ret = self._find_by_location(metadata)
        if ret is not None:
            return ret
        ret = self._find_by_info(metadata)
        if ret is not None:
            return ret
        return None

    def _find_by_condition(self, where_clause, parameters=None):
        query = LrcDb.FIND_LYRIC + where_clause
        logging.debug('Find by condition, query = %s, params = %s' % (query, parameters))
        c = self._conn.cursor()
        c.execute(query, parameters)
        r = c.fetchone()
        logging.debug('Fetch result: %s' % r)
        if r:
            return r[0]
        return None

    def _find_by_location(self, metadata):
        if not metadata.location:
            return None
        return self._find_by_condition(LrcDb.QUERY_LOCATION, (metadata.location,))

    def _find_by_info(self, metadata):
        return self._find_by_condition(LrcDb.QUERY_INFO,
                                       query_param_from_metadata(metadata))

def test():
    """
    >>> import dbus
    >>> from osdlyrics.metadata import Metadata
    >>> db = LrcDb('/tmp/asdf')
    >>> db.assign(Metadata.from_dict({'title': 'Tiger',
    ...                               'artist': 'Soldier',
    ...                               'location': 'file:///tmp/asdf'}),
    ...           'file:///tmp/a.lrc')
    >>> db.find(Metadata.from_dict({'location': 'file:///tmp/asdf'}))
    'file:///tmp/a.lrc'
    >>> db.find(Metadata.from_dict({'location': 'file:///tmp/asdfg'}))
    >>> db.find(Metadata.from_dict({'title': 'Tiger',
    ...                             'location': 'file:///tmp/asdf'}))
    'file:///tmp/a.lrc'
    >>> db.find(Metadata.from_dict({'title': 'Tiger', }))
    >>> db.find(Metadata.from_dict({'title': 'Tiger',
    ...                             'artist': 'Soldier'}))
    'file:///tmp/a.lrc'
    >>> db.assign(Metadata.from_dict({'title': 'ttTiger',
    ...                               'artist': 'ssSoldier',
    ...                               'location': 'file:///tmp/asdf'}),
    ...           'file:///tmp/b.lrc')
    >>> db.find(Metadata.from_dict({'artist': 'Soldier', }))
    >>> db.find(Metadata.from_dict({'title': 'Tiger',
    ...                                      'artist': 'Soldier', }))
    'file:///tmp/b.lrc'
    >>> db.find(Metadata.from_dict({dbus.String('title'): dbus.String('Tiger'),
    ...                             dbus.String('artist'): dbus.String('Soldier'), }))
    'file:///tmp/b.lrc'
    >>> metadata_uni = Metadata.from_dict({'title': '\u6807\u9898', 'artist': '\u6b4c\u624b', })
    >>> db.assign(metadata_uni, '\u8def\u5f84')
    >>> db.find(metadata_uni)
    '\u8def\u5f84'
    >>> db.find(Metadata.from_dict({'title': 'Tiger', 'artist': 'Soldiers', }))
    >>> db.find(Metadata())
    >>> db.delete(Metadata.from_dict({'location': 'file:///tmp/asdf'}))
    >>> db.find(Metadata.from_dict({'title': 'Tiger',
    ...                             'artist': 'Soldier',
    ...                             'location': 'file:///tmp/asdf'}))
    """
    import doctest
    doctest.testmod()

if __name__ == '__main__':
    test()
