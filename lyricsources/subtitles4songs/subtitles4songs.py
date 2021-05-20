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

# import urlparse
import gettext
import html.parser
import http.client
import re

from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import get_proxy_settings, http_download

_ = gettext.gettext

S4S_HOST = 'https://www.rentanadviser.com'
S4S_SEARCH_PATH = '/en/subtitles/subtitles4songs.aspx'
S4S_SUBTITLE_PATH = '/en/subtitles/getsubtitle.aspx'
# Regexes to filter webpage content (search results page, and lyric page).
S4S_SEARCH_RESULT_PATTERN = re.compile(r'<a [^<]*?href="getsubtitle\.aspx(\?artist=.*?song=.*?)">', re.DOTALL)
S4S_LRC_PATTERN = re.compile(r'<span id="ctl00_ContentPlaceHolder1_lbllyrics"><h3>(.*?)<\/h3>(.*?)<\/span>', re.DOTALL)
TITLE_PATTERN = re.compile(r'&song=(.*$)')
ARTIST_PATTERN = re.compile(r'artist=(.*?)&')
BRAND_PATTERN = r"^.*RentAnAdviser\.com.*$"

gettext.bindtextdomain('osdlyrics')
gettext.textdomain('osdlyrics')


class Subtitles4songsSource(BaseLyricSourcePlugin):
    """ Lyric source from https://www.rentanadviser.com/en/subtitles/subtitles4songs.aspx
    """

    def __init__(self):
        super().__init__(id='subtitles4songs', name=_('subtitles4songs'))

    def do_search(self, metadata):
        # Preparing keywords for search.
        keys = []
        if metadata.artist:
            keys.append(metadata.artist)
        if metadata.title:
            keys.append(metadata.title)
        # Joining search terms.
        urlkey = '+'.join(keys).replace(' ', '+')
        # Building the URL.
        url = S4S_HOST + S4S_SEARCH_PATH
        # Request the HTTP page, storing its status and content.
        status, content = http_download(url=url,
                                        params={'q': urlkey},
                                        proxy=get_proxy_settings(self.config_proxy))
        # Checking against HTTP response codes.
        if status < 200 or status >= 400:
            raise http.client.HTTPException(status, '')
        # Filter all occurences of links belonging to search results.
        matches = S4S_SEARCH_RESULT_PATTERN.findall(content.decode('utf8'))
        # Populate osdlyrics' search results for the user to choose from.
        result = []
        if matches:
            for match in matches:
                # Decompose the HTML matches into title, artist and url.
                title = unquote(TITLE_PATTERN.search(match).group(1))
                artist = unquote(ARTIST_PATTERN.search(match).group(1))
                # Build a download URL for this match.
                url = S4S_HOST + S4S_SUBTITLE_PATH + match + "&type=lrc"
                # Add the match metadata for the window with search results.
                if url is not None:
                    result.append(SearchResult(title=title,
                                               artist=artist,
                                               sourceid=self.id,
                                               downloadinfo=url))
        return result

    def do_download(self, downloadinfo):
        status, content = http_download(downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        # Checking against HTTP response codes.
        if status < 200 or status >= 400:
            raise http.client.HTTPException(status)
        # Checking the presence of a HTTP payload.
        if content:
            content = S4S_LRC_PATTERN.search(content.decode('utf8')).group(2)
            # Replacing html breaks with new lines.
            content = re.sub('<br />', "\n", content)
            # Debrand the lyrics.
            content = re.sub(BRAND_PATTERN, " ", content, 0, re.MULTILINE)
        return content.encode('utf-8')


if __name__ == '__main__':
    s4s = Subtitles4songsSource()
    s4s._app.run()
