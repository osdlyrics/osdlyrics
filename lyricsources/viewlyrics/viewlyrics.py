# -*- coding: utf-8 -*-
#
# Copyright (C) 2012 PedroHLC <pedro.laracampos@gmail.com>
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OSD Lyrics. If not, see <http://www.gnu.org/licenses/>.
#

import string
import unicodedata
import httplib
import hashlib
from xml.dom.minidom import parseString
from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import ensure_utf8, http_download, get_proxy_settings

VIEWLYRICS_HOST = 'search.crintsoft.com'
VIEWLYRICS_SEARCH_URL = '/searchlyrics.htm'
VIEWLYRICS_BASE_LRC_URL = 'http://viewlyrics.com/'

VIEWLYRICS_QUERY_FORM = '<?xml version=\'1.0\' encoding=\'utf-8\' ?><searchV1 artist=\"%artist\" title=\"%title\"%etc />'
VIEWLYRICS_AGENT = 'MiniLyrics'
VIEWLYRICS_KEY = 'Mlv1clt4.0'

def normalize_str(s):
    """ If s is a unicode string, only keep alphanumeric characters and remove
        diacritics
    """
    try:
        return ''.join(x for x in unicodedata.normalize('NFKD', s)
                       if x in string.ascii_letters or x in string.digits).lower()
    except TypeError:
        return s

class ViewlyricsSource(BaseLyricSourcePlugin):
    def __init__(self):
        
        BaseLyricSourcePlugin.__init__(self, id='viewlyrics', name='ViewLyrics')

    def do_search(self, metadata):
        if metadata.title:
            title =  metadata.title
        else:
            title =  ''
        if metadata.artist:
            artist = metadata.artist
        else:
            artist = ''
        
        result = []
        page = 0
        pagesleft = 1
        while(pagesleft > 0):
            pageresult, pagesleft = self.real_search(title, artist, page)
            result += pageresult
            page += 1

        # Remove non-lrc (plain text) results, they cannot be displayed by
        # OSDLyrics for now
        def res_is_lrc(result):
            url = result._downloadinfo
            return url.rfind('lrc') == len(url) - 3
        result = filter(res_is_lrc, result)

        # Prioritize results whose artist matches
        if metadata.artist and metadata.title:
            n_artist = normalize_str(artist)
            def res_has_same_artist(result):
                return normalize_str(result._artist) == n_artist
            result.sort(key=res_has_same_artist, reverse=True)

        return result

    def real_search(self, title='', artist='', page = 0):
        query = VIEWLYRICS_QUERY_FORM
        query =  query.replace('%title', title)
        query =  query.replace('%artist', artist)
        query =  ensure_utf8(query.replace('%etc', ' client=\"MiniLyrics\" RequestPage=\'%d\'' % page)) #Needs real RequestPage
        
        queryhash = hashlib.md5()
        queryhash.update(query)
        queryhash.update(VIEWLYRICS_KEY)
        
        masterquery = '\2\0\4\0\0\0' + queryhash.digest() + query
        
        url = VIEWLYRICS_HOST + VIEWLYRICS_SEARCH_URL
        status, content = http_download(url=url,
                                        method='POST',
                                        params=masterquery,
                                        proxy=get_proxy_settings(self.config_proxy))
        
        if status < 200 or status >= 400:
                raise httplib.HTTPException(status, '')
        
        contentbytes = map(ord, content)
        codekey = contentbytes[1]
        deccontent = ''
        for char in contentbytes[22:]:
                deccontent += unichr(char ^ codekey)
        
        result = []
        pagesleft = 0
        tagreturn = parseString(deccontent).getElementsByTagName('return')[0]
        if tagreturn:
                pagesleftstr = self.alternative_gettagattribute(tagreturn.attributes.items(), 'PageCount') #tagreturn.attributes['PageCount'].value
                if pagesleftstr == '':
                    pagesleft = 0
                else:
                    pagesleft = int(pagesleftstr)
                tagsfileinfo = tagreturn.getElementsByTagName('fileinfo')
                if tagsfileinfo:
                    for onefileinfo in tagsfileinfo:
                        if onefileinfo.hasAttribute('link'):
                            title = onefileinfo.getAttribute('title')
                            artist = onefileinfo.getAttribute('artist')
                            album = onefileinfo.getAttribute('album')
                            url = VIEWLYRICS_BASE_LRC_URL + onefileinfo.getAttribute('link')
                            result.append(SearchResult(title=title,
                                                       artist=artist,
                                                       album=album,
                                                       sourceid=self.id,
                                                       downloadinfo=url))
        return result, (pagesleft - page)

    def alternative_gettagattribute(self, attrs, key):
        key = key.lower()
        for attrName, attrValue in attrs:
            if attrName.lower() == key:
                return attrValue
        return ''

    def do_download(self, downloadinfo):
        # return a string
        # downloadinfo is what you set in SearchResult
        if not isinstance(downloadinfo, str) and \
                not isinstance(downloadinfo, unicode):
            raise TypeError('Expect the downloadinfo as a string of url, but got type ',
                            type(downloadinfo))
        status, content = http_download(url=downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status, '')
        return content

if __name__ == '__main__':
    viewlyrics = ViewlyricsSource()
    viewlyrics._app.run()
