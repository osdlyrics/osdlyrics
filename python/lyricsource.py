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

import threading
import dbus
import logging

from consts import \
    LYRIC_SOURCE_PLUGIN_BUS_NAME_PREFIX, \
    LYRIC_SOURCE_PLUGIN_INTERFACE, \
    LYRIC_SOURCE_PLUGIN_OBJECT_PATH_PREFIX
from dbusext.service import Object as DBusObject, property as dbus_property
from app import App
from metadata import Metadata
from config import Config

SEARCH_SUCCEED = 0
SEARCH_CANCELLED = 1
SEARCH_FAILED = 2

DOWNLOAD_SUCCEED = 0
DOWNLOAD_CANCELLED = 1
DOWNLOAD_FAILED = 2

__all__ = (
    'BaseLyricSourcePlugin',
    'SearchResult',
    )

def onmainthread(func):
    def decfunc(self, app, *args, **kwargs):
        def timeout_cb():
            func(self, *args, **kwargs)
            return False
        app.run_on_main_thread(timeout_cb)
    return decfunc

class SearchResult(object):
    """ Lyrics that match the metadata to be searched.
    """
    def __init__(self, sourceid, downloadinfo, title='', artist='', album='', comment=''):
        """

        Arguments:
        - `title`: The matched lyric title.
        - `artist`: The matched lyric artist.
        - `album`: The matched lyric album.
        - `downloadinfo`: Some additional data that is needed to download the
          lyric. Normally this value is the url or ID of the lyric.
          ``downloadinfo`` MUST be composed with basic types such as numbers,
          lists, dicts or strings so that it can be converted to D-Bus compatible
          dict with `to_dict` method.
        """
        self._title = title
        self._artist = artist
        self._album = album
        self._comment = comment
        self._sourceid = sourceid
        self._downloadinfo = downloadinfo

    def to_dict(self, ):
        """ Convert the result to a dict so that it can be sent with D-Bus.
        """
        return { 'title': self._title,
                 'artist': self._artist,
                 'album': self._album,
                 'comment': self._comment,
                 'sourceid': self._sourceid,
                 'downloadinfo': self._downloadinfo }

class BaseTaskThread(threading.Thread):
    """ Base thread for search or download tasks.

    Plugins MUST provide a callable object as the `target` argument in the
    initializer. The target does the task and returns the results. If task
    fails, an Exception SHOULD be raised in the target.
    """

    def __init__(self, onfinish, onerror, target, args=(), kwargs={}):
        """
        Initialize the thread. The main thread should provide to callbacks
        to notify the main thread that the thread is finished or an error
        occurs. Please note that the two callbacks are run in the new thread,
        not the main thread. So the callback may notify the main thread to
        handle the results with App.run_on_main_thread.

        Arguments:

        - `onfinish`: A callable object to be invoked when `target` finish.
          The callable SHOULD receive the values returned by `do_task`.
        - `onerror`: A callable object to be invoked when `target` raise an
          exception. The callable SHOULD receive the exception raised by
          `do_task`.
        - `target`: The callable object to be invoked by the run() method.
        - `args`: The argument tuple for the target invocation. Defaults to `()`.
        - `kwargs`: A dictionary of keyword arguments for the target invocation.
          Defaults to `{}`.
        """
        self._onfinish = onfinish
        self._onerror = onerror
        self._args = args
        self._kwargs = kwargs
        self._target = target
        threading.Thread.__init__(self)

    def run(self):
        """ Runs the task thread. Do NOT override this method. Override
        `do_task` instead.
        """
        try:
            ret = self._target(*self._args, **self._kwargs)
            self._onfinish(ret)
        except Exception as e:
            logging.exception('Got exception in thread')
            self._onerror(e)
        import sys
        sys.stdout.flush()

class BaseLyricSourcePlugin(DBusObject):
    """ Base class for implementing a lyric source plugin
    """

    def __init__(self, id, name=None, watch_daemon=True):
        """
        Create a new lyric source instance.

        Arguments:

        - `id`: The unique ID of the lyric source plugin. The full bus
          name of the plugin will be `org.osdlyrics.LyricSourcePlugin.id`
        - `name`: (optional) The name of the plugin, which should be properly
          localized. If `name` is missing, the plugin will take `id` as its
          name.
        - `watch_daemon`: Whether to watch daemon bus.
        """
        self._id = id
        self._app = App('LyricSourcePlugin.' + id,
                        watch_daemon=watch_daemon)
        DBusObject.__init__(self,
                            conn=self._app.connection,
                            object_path=LYRIC_SOURCE_PLUGIN_OBJECT_PATH_PREFIX + self._id)
        self._search_count = 0
        self._download_count = 0
        self._search_tasks = {}
        self._download_tasks = {}
        self._name = name if name is not None else id
        self._config = None

    def do_search(self, metadata):
        """
        Do the real search work by plugins. All plugins MUST implement this method.

        This method runs in a seperate thread, so don't worry about block IO.

        Parameters:

        - `metadata`: The metadata of the track to search. The type of `metadata`
          is osdlyrics.metadata.Metadata

        Returns: A list of SearchResult objects
        """
        raise NotImplementedError()

    @onmainthread
    def do_searchsuccess(self, ticket, results):
        if ticket in self._search_tasks:
            del self._search_tasks[ticket]
            dbusresults = [result.to_dict() for result in results]
            self.SearchComplete(ticket, SEARCH_SUCCEED, dbusresults)

    @onmainthread
    def do_searchfailure(self, ticket, e):
        if ticket in self._search_tasks:
            del self._search_tasks[ticket]
            logging.info('Search fail, %s' % e)
            self.SearchComplete(ticket, SEARCH_FAILED, [])

    @dbus.service.method(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                         in_signature='a{sv}',
                         out_signature='i')
    def Search(self, metadata):
        ticket = self._search_count
        self._search_count = self._search_count + 1
        thread = BaseTaskThread(onfinish=lambda result: self.do_searchsuccess(self._app, ticket, result),
                                onerror=lambda e: self.do_searchfailure(self._app, ticket, e),
                                target=self.do_search,
                                kwargs={'metadata': Metadata.from_dict(metadata)})
        self._search_tasks[ticket] = thread
        thread.start()
        return ticket

    @dbus.service.method(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                         in_signature='i',
                         out_signature='')
    def CancelSearch(self, ticket):
        if ticket in self._search_tasks:
            del self._search_tasks[ticket]
            self.SearchComplete(ticket, SEARCH_CANCELLED, [])


    def do_download(self, downloadinfo):
        """
        Do the real download work by plugins. All plugins MUST implement this
        method.

        This method runs in a seperate thread, so don't worry about block IO.

        Parameters:

        - `downloadinfo`: The additional info taken from `downloadinfo` field in
          SearchResult objects.

        Returns: A string of the lyric content
        """
        raise NotImplementedError()

    @onmainthread
    def do_downloadsuccess(self, ticket, content):
        if ticket in self._download_tasks:
            del self._download_tasks[ticket]
            self.DownloadComplete(ticket, DOWNLOAD_SUCCEED, str(content))

    @onmainthread
    def do_downloadfailure(self, ticket, e):
        if ticket in self._download_tasks:
            del self._download_tasks[ticket]
            self.DownloadComplete(ticket, DOWNLOAD_FAILED, str(e))

    @dbus.service.method(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                         in_signature='v',
                         out_signature='i')
    def Download(self, downloadinfo):
        ticket = self._download_count
        self._download_count = self._download_count + 1
        thread = BaseTaskThread(onfinish=lambda content: self.do_downloadsuccess(self._app, ticket, content),
                                onerror=lambda e: self.do_downloadfailure(self._app, ticket, e),
                                target=self.do_download,
                                kwargs={'downloadinfo': downloadinfo})
        self._download_tasks[ticket] = thread
        thread.start()
        return ticket

    @dbus.service.method(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                         in_signature='i',
                         out_signature='')
    def CancelDownload(self, ticket):
        if ticket in self._download_tasks:
            del self._download_tasks[ticket]
            self.DownloadComplete(ticket, DOWNLOAD_CANCELLED, '')

    @dbus_property(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                  type_signature='s')
    def Name(self):
        return self._name

    @dbus.service.signal(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                         signature='iiaa{sv}')
    def SearchComplete(self, ticket, status, results):
        logging.debug('search complete: ticket: %d, status: %d' % (ticket, status))
        pass


    @dbus.service.signal(dbus_interface=LYRIC_SOURCE_PLUGIN_INTERFACE,
                         signature='iiay')
    def DownloadComplete(self, ticket, status, result):
        logging.debug('download complete: ticket: %d, status: %d' % (ticket, status), '' if status == DOWNLOAD_SUCCEED else ', result: %s' % result)
        pass

    def run(self):
        """
        Run the plugin as a standalone application
        """
        self._app.run()

    @property
    def app(self):
        """
        Return the App object that the plugin uses.
        """
        return self._app

    @property
    def id(self):
        """
        Return the ID of the lyric source
        """
        return self._id

    @property
    def config_proxy(self):
        if self._config is None:
            self._config = Config(self._app.connection)
        return self._config

def test():
    class DummyLyricSourcePlugin(BaseLyricSourcePlugin):
        def __init__(self):
            BaseLyricSourcePlugin.__init__(self,
                                           id='dummy',
                                           watch_daemon=False)

        def do_search(self, metadata):
            if metadata.title:
                logging.info('title: %s' % metadata.title)
                results = [SearchResult(title=metadata.title + str(i),
                                        artist=metadata.artist + str(i),
                                        album=metadata.album + str(i),
                                        downloadinfo='\n'.join((metadata.title,
                                                               metadata.artist,
                                                               metadata.album)))
                           for i in xrange(10)]
                return results

            raise Exception('Title must not be empty')

        def do_download(self, downloadinfo):
            if isinstance(downloadinfo, str) or isinstance(downloadinfo, unicode):
                return downloadinfo
            else:
                raise Exception('downloadinfo should be a string')

    search_tickets = {}
    download_tickets = {}

    def search_reply(ticket, expect_status):
        if ticket in search_tickets:
            logging.warning('Error: search ticket %d exists', ticket)
        else:
            search_tickets[ticket] = expect_status

    def search_complete_cb(ticket, status, results):
        if ticket not in search_tickets:
            logging.warning('Error! search ticket not exists')
            return

        if search_tickets[ticket] != status:
            logging.warning('Error! expect search %d with status %d but %d got',
                            ticket, search_tickets[ticket], status)
            return
        logging.debug('Search #%d with status %d', ticket, status)
        if status == 0:
            downloadinfo = results[0]['downloadinfo']
            source.Download(downloadinfo,
                            reply_handler=lambda t:download_reply(t, 0),
                            error_handler=dummy_error)
        del search_tickets[ticket]
        check_quit()

    def download_reply(ticket, expect_status):
        if ticket in download_tickets:
            logging.warning('Error: download ticket %d already exists', ticket)
        else:
            download_tickets[ticket] = expect_status

    def download_complete_cb(ticket, status, content):
        if ticket not in download_tickets:
            logging.warning('Error! download ticket not exists')
            return

        if download_tickets[ticket] != status:
            logging.warning('Error! expect download status %d but %d got', download_tickets[ticket], status)
            return
        if status == 0:
            logging.debug('Download #%d success', ticket)
            logging.debug('Downloaded content: \n%s', ''.join([chr(b) for b in content]))
        else:
            logging.warning('Download #%d fail, msg: %s', ticket, ''.join([chr(b) for b in content]))
        del download_tickets[ticket]
        check_quit()

    wait_count = [4]

    def check_quit():
        wait_count[0] -= 1
        if wait_count[0] == 0:
            app.quit()

    def dummy_error(e):
        logging.warning('Error: ' + e)

    dummysource = DummyLyricSourcePlugin()
    app = dummysource.app
    conn = app.connection
    source = conn.get_object('org.osdlyrics.LyricSourcePlugin.dummy',
                             '/org/osdlyrics/LyricSourcePlugin/dummy')
    source.connect_to_signal('SearchComplete',
                             search_complete_cb)
    source.connect_to_signal('DownloadComplete',
                             download_complete_cb)
    source.Search({'title': 'dummytitle',
                   'artist': 'dummyartist',
                   'album': 'dummyalbum'},
                  reply_handler=lambda t: search_reply(t, 0),
                  error_handler=dummy_error)
    source.Search({'foo': 'bar'},
                  reply_handler=lambda t:search_reply(t, 2),
                  error_handler=dummy_error)
    source.Download(123,
                    reply_handler=lambda t:download_reply(t, 2),
                    error_handler=dummy_error)
    app.run()

if __name__ == '__main__':
    test()
