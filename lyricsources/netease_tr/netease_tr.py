from lyricsources.netease import NeteaseSource

class NeteaseTranslatedSource(NeteaseSource):
    def __init__(self):
        super().__init__(attempt_use_translation=True)

if __name__ == '__main__':
    neteasetr = NeteaseTranslatedSource()
    neteasetr._app.run()
