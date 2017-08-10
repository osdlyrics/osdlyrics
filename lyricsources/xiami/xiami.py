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

import re
import httplib
# import urlparse
import gettext
import HTMLParser
from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import ensure_utf8, http_download, get_proxy_settings

_ = gettext.gettext

XIAMI_HOST = 'www.xiami.com'
XIAMI_SEARCH_URL = '/search'
XIAMI_SONG_URL = '/song/'
XIAMI_LRC_URL = '/song/playlist/id/'
XIAMI_SEARCH_PATTERN = re.compile(r'(<a [^<]*?href="http://www.xiami.com/song/(\w+)".*?>).*?(<a [^<]*?href="http://www.xiami.com/artist/.*?>).*?(<a [^<]*?href="http://www.xiami.com/album/.*?>)', re.DOTALL)
XIAMI_ID_PATTERN = re.compile(r'<a [^<]*?onclick="tag\((\d+).*?>')
XIAMI_URL_PATTERN = re.compile(r'<lyric>(.*?)</lyric>', re.DOTALL)
TITLE_ATTR_PATTERN = re.compile(r'title="(.*?)"')

gettext.bindtextdomain('osdlyrics')
gettext.textdomain('osdlyrics')

class XiamiSource(BaseLyricSourcePlugin):
    """ Lyric source from xiami.com
    """

    def __init__(self):
        """
        """

        BaseLyricSourcePlugin.__init__(self, id='xiami', name=_('Xiami'))
        self._search = {}
        self._download = {}

    def do_search(self, metadata):
        keys = []
        if metadata.title:
            keys.append(metadata.title)
        if metadata.artist:
            keys.append(metadata.artist)
        urlkey = ensure_utf8('+'.join(keys)).replace(' ', '+')
        url = XIAMI_HOST + XIAMI_SEARCH_URL
        status, content = http_download(url=url,
                                        params={'key': urlkey},
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status, '')
        match = XIAMI_SEARCH_PATTERN.findall(content)
        result = []
        if match:
            for title_elem, id, artist_elem, album_elem in match:
                title = TITLE_ATTR_PATTERN.search(title_elem).group(1)
                artist = TITLE_ATTR_PATTERN.search(artist_elem).group(1)
                album = TITLE_ATTR_PATTERN.search(album_elem).group(1)
                url = self.get_url(id)
                if url is not None:
                    result.append(SearchResult(title=title,
                                               artist=artist,
                                               album=album,
                                               sourceid=self.id,
                                               downloadinfo=url))
        return result

    def get_songid(self, id):
        status, content = http_download(url=XIAMI_HOST + XIAMI_SONG_URL + str(id),
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            return None
        match = XIAMI_ID_PATTERN.search(content)
        if not match:
            return None
        songid = match.group(1).strip()
        return songid

    def get_url(self, id):
        songid = self.get_songid(id)
        status, content = http_download(url=XIAMI_HOST + XIAMI_LRC_URL + str(songid),
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            return None
        match = XIAMI_URL_PATTERN.search(content)
        if not match:
            return None
        url = match.group(1).strip()
        if url.lower().endswith('.lrc'):
            return url
        else:
            return None

    def do_download(self, downloadinfo):
        if not isinstance(downloadinfo, str) and \
                not isinstance(downloadinfo, unicode):
            raise TypeError('Expect the downloadinfo as a string of url, but got type ',
                            type(downloadinfo))
        # parts = urlparse.urlparse(downloadinfo)
        status, content = http_download(downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status)
        if content:
            content = HTMLParser.HTMLParser().unescape(content.decode('utf-8'))
        return content.encode('utf-8')

if __name__ == '__main__':
    xiami = XiamiSource()
    xiami._app.run()
