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
# along with OSD Lyrics.  If not, see <https://www.gnu.org/licenses/>.
#

from future import standard_library
standard_library.install_aliases()
from builtins import str, super
from urllib.parse import unquote

import urllib.parse
import gettext
import html.parser
import html
import http.client
import re
import os

from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import get_proxy_settings, http_download

_ = gettext.gettext

MEGALOBIZ_HOST = 'https://www.megalobiz.com'
MEGALOBIZ_SEARCH_PATH = '/search/all'
MEGALOBIZ_LYRIC_PATH = '/lrc/maker/'
MEGALOBIZ_DOWNLOAD_PATH = '/lrc/increment-downloads'
# Regexes to filter webpage content (search results page, and lyric page).
MEGALOBIZ_SEARCH_RESULT_PATTERN = re.compile(r'<a class=\"entity_name\".*?id=\"([0-9]+)\".*?name=\"(.*?)\".*?href=\"(.*?)\".+?</a>', re.DOTALL)
MEGALOBIZ_TITLE_ARTIST_SPLIT_PATTERN = re.compile(r'(.*?)(\sby\s|\s\-\s)(.*)', re.DOTALL)
MEGALOBIZ_LRC_PATTERN = re.compile(r'lyrics_details.*?<span.*?>(.*?)<\/span>', re.DOTALL)

gettext.bindtextdomain('osdlyrics')
gettext.textdomain('osdlyrics')


class MegalobizSource(BaseLyricSourcePlugin):
    """ Lyric source from https://www.megalobiz.com/
    """

    def __init__(self):
        super().__init__(id='megalobiz', name=_('megalobiz'))

    def do_search(self, metadata):
        # Preparing keywords for search.
        keys = []
        if metadata.artist:
            keys.append('"' + metadata.artist + '"')
        if metadata.title:
            keys.append('"' + metadata.title + '"')
        # Joining search terms.
        urlkey = '+'.join(keys).replace(' ', '+')
        # Building the URL.
        url = MEGALOBIZ_HOST + MEGALOBIZ_SEARCH_PATH
        # Request the HTTP page, storing its status and content.
        status, content = http_download(url=url,
                                        params={'qry': urlkey},
                                        proxy=get_proxy_settings(self.config_proxy))
        # Checking against HTTP response codes.
        if status < 200 or status >= 400:
            raise http.client.HTTPException(status, '')
        # Filter all occurences of links belonging to search results.
        matches = MEGALOBIZ_SEARCH_RESULT_PATTERN.findall(content.decode('utf8'))
        # Populate osdlyrics' search results for the user to choose from.
        result = []
        if matches:
            for match in matches:
                # The first three groups in a match are
                # lyric_id, full_name (artist + song title) and lyric path.
                # lyric_id = match[0]
                full_name = match[1]
                path = match[2]
                # Decompose optimistically the full_name into artist and song title
                name_parts = MEGALOBIZ_TITLE_ARTIST_SPLIT_PATTERN.findall(full_name)
                if isinstance(name_parts, list) and len(name_parts) > 0:
                    name_parts = len(name_parts) > 0 and name_parts[0] or exit()
                    title = name_parts[1] == ' by ' and name_parts[0] or name_parts[2]
                    artist = name_parts[1] == ' by ' and name_parts[2] or name_parts[0]
                # In case we can't split, use the full string as title.
                else:
                    title = full_name
                    artist = ''
                title = html.unescape(title)
                artist = html.unescape(artist)
                lyric_url = MEGALOBIZ_HOST + path
                if path is not None:
                    result.append(SearchResult(title=title,
                                               artist=artist,
                                               sourceid=self.id,
                                               downloadinfo=lyric_url))
        return result

    def do_download(self, downloadinfo):
        ''' Download handler for this data source.
        '''
        status, content = http_download(downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        # Checking against HTTP response codes.
        if status < 200 or status >= 400:
            raise http.client.HTTPException(status)
        # Checking the presence of a HTTP payload.
        if content:
            content = MEGALOBIZ_LRC_PATTERN.search(content.decode('utf-8')).group(1)
            # Replacing html breaks with new lines.
            content = re.sub('<br>', "\n", content)
            # Replace html entities present in the lyric content.
            content = html.unescape(content)
        return content.encode('utf-8')


if __name__ == '__main__':
    mls = MegalobizSource()
    mls._app.run()
