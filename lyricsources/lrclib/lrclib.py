# -*- coding: utf-8 -*-
#
# Copyright (C) 2025 OSD Lyrics Contributors
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

import json
import urllib.parse
import gettext
import http.client

from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import get_proxy_settings, http_download

_ = gettext.gettext

# API endpoints
LRCLIB_HOST = 'https://lrclib.net'
LRCLIB_SEARCH_PATH = '/api/search'
LRCLIB_GET_PATH = '/api/get'
LRCLIB_GET_CACHED_PATH = '/api/get-cached'

# Set User-Agent as recommended in the API docs
USER_AGENT = 'OSDLyrics v0.5.15 (https://github.com/osdlyrics/osdlyrics)'

gettext.bindtextdomain('osdlyrics')
gettext.textdomain('osdlyrics')


class LrcLibSource(BaseLyricSourcePlugin):
    """ Lyric source from https://lrclib.net/
    """

    def __init__(self):
        super().__init__(id='lrclib', name=_('LrcLib'))

    def do_search(self, metadata):
        """
        Search for lyrics using LrcLib API
        """
        # Prepare search parameters
        params = {}
        
        # If both title and artist are available, use them for search
        if metadata.title and metadata.artist:
            params['track_name'] = metadata.title
            params['artist_name'] = metadata.artist
        # If only title is available, use it as a general query
        elif metadata.title:
            params['q'] = metadata.title
        # If only artist is available, use it as a general query
        elif metadata.artist:
            params['q'] = metadata.artist
        else:
            # No search criteria available
            return []
            
        # Add album name if available
        if metadata.album:
            params['album_name'] = metadata.album
            
        # Build the URL and make request
        url = LRCLIB_HOST + LRCLIB_SEARCH_PATH
        headers = {'User-Agent': USER_AGENT}
        status, content = http_download(
            url=url,
            params=params,
            headers=headers,
            proxy=get_proxy_settings(self.config_proxy)
        )
        
        # Check HTTP response code
        if status < 200 or status >= 400:
            if status == 404:
                # Not found is a normal condition, return empty list
                return []
            raise http.client.HTTPException(status, content)
            
        # Parse JSON response
        results = json.loads(content.decode('utf-8'))
            
        # Process search results
        search_results = []
        for result in results:
            # Use the ID as downloadinfo - this is what we'll use to fetch lyrics
            track_id = result.get('id')
            if track_id:
                search_results.append(
                    SearchResult(
                        title=result.get('trackName', ''),
                        artist=result.get('artistName', ''),
                        album=result.get('albumName', ''),
                        sourceid=self.id,
                        downloadinfo=str(track_id)
                    )
                )
            
        return search_results

    def do_download(self, downloadinfo):
        """
        Download lyrics using LrcLib API
        """
        # downloadinfo is now just the track ID as a string
        url = f"{LRCLIB_HOST}{LRCLIB_GET_PATH}/{downloadinfo}"
        headers = {'User-Agent': USER_AGENT}
        status, content = http_download(
            url=url,
            headers=headers,
            proxy=get_proxy_settings(self.config_proxy)
        )
            
        # Check HTTP response code
        if status < 200 or status >= 400:
            raise http.client.HTTPException(status, content)
            
        # Parse JSON response
        result = json.loads(content.decode('utf-8'))
        
        # Check if we have synced lyrics
        if result.get('syncedLyrics'):
            return result['syncedLyrics'].encode('utf-8')
        # Fall back to plain lyrics if no synced lyrics
        elif result.get('plainLyrics'):
            return result['plainLyrics'].encode('utf-8')
        # Return empty string if no lyrics found
        else:
            return b''


if __name__ == '__main__':
    lls = LrcLibSource()
    lls._app.run()