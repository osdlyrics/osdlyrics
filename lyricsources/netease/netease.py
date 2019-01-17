import httplib
import gettext
import json
from osdlyrics.lyricsource import BaseLyricSourcePlugin, SearchResult
from osdlyrics.utils import ensure_utf8, http_download, get_proxy_settings

_ = gettext.gettext

NETEASE_HOST = 'music.163.com'
NETEASE_SEARCH_URL = '/api/search/get'
NETEASE_LYRIC_URL = '/api/song/lyric'

gettext.bindtextdomain('osdlyrics')
gettext.textdomain('osdlyrics')

class NeteaseSource(BaseLyricSourcePlugin):
    """ Lyric source from music.163.com
    """

    def __init__(self):
        BaseLyricSourcePlugin.__init__(self, id='netease', name=_('Netease'))

    def do_search(self, metadata):
        # type: (osdlyrics.metadata.Metadata) -> List[SearchResult]
        keys = []
        if metadata.title:
            keys.append(metadata.title)
        if metadata.artist:
            keys.append(metadata.artist)
        urlkey = ensure_utf8('+'.join(keys)).replace(' ', '+')
        url = NETEASE_HOST + NETEASE_SEARCH_URL
        params = 's=%s&type=1' % urlkey

        status, content = http_download(url=url,
                                        method='POST',
                                        params=params,
                                        proxy=get_proxy_settings(self.config_proxy))

        if status < 200 or status >= 400:
            raise httplib.HTTPException(status, '')

        def map_func(song):
            if len(song['artists']) > 0:
                artist_name = song['artists'][0]['name']
            else:
                artist_name = ''
            url = NETEASE_HOST + NETEASE_LYRIC_URL + '?id=' + str(song['id']) + '&lv=-1&kv=-1&tv=-1'
            return SearchResult(title=song['name'],
                                artist=artist_name,
                                album=song['album']['name'],
                                sourceid=self.id,
                                downloadinfo=url)

        parsed = json.loads(content.decode('utf-8'))
        result = list(map(map_func, parsed['result']['songs']))

        return result

    def do_download(self, downloadinfo):
        # type: (Any) -> bytes
        status, content = http_download(url=downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status)

        parsed = json.loads(content.decode('utf-8'))
        if 'nolyric' in parsed:
            raise ValueError('This item has no lyrics.')
        lyric = parsed['lrc']['lyric']
        return lyric.encode('utf-8')


if __name__ == '__main__':
    netease = NeteaseSource()
    netease._app.run()
