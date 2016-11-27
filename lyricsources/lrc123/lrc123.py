# -*- coding: utf-8 -*-
#
# Copyright (C) 2012 Tiger Soldier <tigersoldi@gmail.com>
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

import httplib
import logging
import re

import pycurl

from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import http_download, get_proxy_settings

HOST = 'www.lrc123.com'
SEARCH_URL = '/?keyword=%s&field=all'
RESULT_PATTERN = re.compile(r'<div class="newscont .*?href="/\?field=singer.*?>(.*?)</a>.*?href="/\?field=album.*?>(.*?)</a>.*?href="/\?field=song.*?>(.*?)</a>.*?href="/download/lrc/(.*?)">LRC', re.DOTALL)
DOWNLOAD_URL_PREFIX = '/download/lrc/'

class Lrc123Source(BaseLyricSourcePlugin):
    """ Lyric source from xiami.com
    """

    def __init__(self):
        """
        """

        BaseLyricSourcePlugin.__init__(self, id='lrc123', name='LRC123')

    def do_search(self, metadata):
        keys = []
        if metadata.title:
            keys.append(metadata.title)
        if metadata.artist:
            keys.append(metadata.artist)
        urlkey = (' '.join(keys))
        params = {'keyword': urlkey,
                  'field': 'all'}
        try:
            status, content = http_download(
                url=HOST + '/',
                params=params,
                proxy=get_proxy_settings(config=self.config_proxy))
        except pycurl.error as e:
            logging.error('Download failed. %s', e.args[1])
            return []

        if status < 200 or status >= 400:
            raise httplib.HTTPException(status)
        match = RESULT_PATTERN.findall(content)
        result = []
        if match:
            for artist, album, title, url in match:
                title = title.replace('<span class="highlighter">', '').replace('</span>', '')
                artist = artist.replace('<span class="highlighter">', '').replace('</span>', '')
                album = album.replace('<span class="highlighter">', '').replace('</span>', '')
                url = DOWNLOAD_URL_PREFIX + url
                result.append(SearchResult(title=title,
                                           artist=artist,
                                           album=album,
                                           sourceid=self.id,
                                           downloadinfo=url))
        return result

    def do_download(self, downloadinfo):
        if not isinstance(downloadinfo, str) and \
                not isinstance(downloadinfo, unicode):
            raise TypeError('Expect the downloadinfo as a string of url, but got type ',
                            type(downloadinfo))
        status, content = http_download(url=HOST+downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status, '')
        return content

if __name__ == '__main__':
    lrc123 = Lrc123Source()
    lrc123._app.run()
