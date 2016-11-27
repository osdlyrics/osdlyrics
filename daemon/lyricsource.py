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
# along with OSD Lyrics.  If not, see <http://www.gnu.org/licenses/>.
#

import dbus
import logging

import osdlyrics
import osdlyrics.config

STATUS_SUCCESS = 0
STATUS_CANCELLED = 1
STATUS_FAILURE = 2


def validateticket(component):
    def decorator(func):
        def dec_func(self, source_id, ticket, *args, **kwargs):
            if source_id not in self._sources:
                logging.warning('%s is not in source list' % source_id)
                return
            source = self._sources[source_id]
            if ticket not in source[component]:
                logging.warning('%s is not valid %s ticket of source %s' % (ticket, component, source_id))
                return
            func(self, source_id, ticket, *args, **kwargs)
        return dec_func
    return decorator


class LyricSource(dbus.service.Object):
    """ Implement org.osdlyrics.LyricSource interface
    """

    def __init__(self, conn):
        dbus.service.Object.__init__(self,
                                     conn=conn,
                                     object_path=osdlyrics.LYRIC_SOURCE_OBJECT_PATH)
        self._sources = {}
        self._search_tasks = {}
        self._n_search_tickets = 0
        self._download_tasks = {}
        self._n_download_tickets = 0
        self._detect_sources()
        self._config = osdlyrics.config.Config(conn)

    def _detect_sources(self):
        for bus_name in self.connection.list_names():
            try:
                self._connect_source(bus_name, False)
            except Exception as e:
                logging.warning('Fail to connect source %s: %s' % (bus_name, e))
        for bus_name in self.connection.list_activatable_names():
            try:
                self._connect_source(bus_name, True)
            except Exception as e:
                logging.warning('Fail to connect source %s: %s' % (bus_name, e))

    def _connect_source(self, bus_name, activate):
        if not bus_name.startswith(osdlyrics.LYRIC_SOURCE_PLUGIN_BUS_NAME_PREFIX):
            return
        logging.info('Connecting to lyric source %s' % bus_name)
        source_id = bus_name[len(osdlyrics.LYRIC_SOURCE_PLUGIN_BUS_NAME_PREFIX):]
        if source_id in self._sources:
            return
        if activate:
            try:
                self.connection.activate_name_owner(bus_name)
            except Exception as e:
                logging.warning('Cannot activate lyric source %s: %s' % (bus_name, e))
                return
        path = osdlyrics.LYRIC_SOURCE_PLUGIN_OBJECT_PATH_PREFIX + source_id
        proxy = dbus.Interface(self.connection.get_object(bus_name, path),
                               osdlyrics.LYRIC_SOURCE_PLUGIN_INTERFACE)
        property_iface = dbus.Interface(proxy, 'org.freedesktop.DBus.Properties')
        source = {
            'proxy': proxy,
            'name': property_iface.Get(osdlyrics.LYRIC_SOURCE_PLUGIN_INTERFACE,
                                       'Name'),
            'id': source_id,
            'search': {},
            'download': {},
            }
        self._sources[source_id] = source
        proxy.connect_to_signal('SearchComplete',
                                lambda t, s, r: self.search_complete_cb(source_id,
                                                                        t, s, r))
        proxy.connect_to_signal('DownloadComplete',
                                lambda t, s, c: self.download_complete_cb(source_id,
                                                                          t, s, c))

    @validateticket('search')
    def search_complete_cb(self, source_id, ticket, status, results):
        logging.info('Search complete from %s, ticket: %s, status: %s, result: %s' %
                     (source_id, ticket, status, len(results)))
        source = self._sources[source_id]
        myticket = source['search'][ticket]
        del source['search'][ticket]
        if myticket not in self._search_tasks:
            return
        if status == STATUS_SUCCESS:
            mytask = self._search_tasks[myticket]
            mytask['failure'] = False
        if (status == STATUS_SUCCESS and len(results) > 0) or \
                status == STATUS_CANCELLED:
            self.SearchComplete(myticket, status, results)
        else: #STATUS_FAILURE
            mytask = self._search_tasks[myticket]
            # mytask['failure'] is set to True only when all sources fail to search.
            # To ensure that, we set this value to None when task is created, and
            # set it to False when a search task succeeds, thus `not task['failure']`
            # will return True. The value is set to True only when it was not False,
            # so no search tasks from other sources succeed.
            if status == STATUS_FAILURE and mytask['failure'] != False:
                mytask['failure'] = True
            if len(mytask['sources']) == 0 or mytask['sources'][0] != source_id:
                logging.warning('Error, no source exists or source id mismatch with current id')
                self.SearchComplete(myticket, STATUS_FAILURE, results)
            else:
                mytask['sources'].pop(0)
                self._do_search(myticket)

    @validateticket('download')
    def download_complete_cb(self, source_id, ticket, status, content):
        logging.info('Download complete from %s, ticket: %s, status: %s, content length: %s' %
                     (source_id, ticket, status, len(content)))
        source = self._sources[source_id]
        myticket = source['download'][ticket]
        del source['download'][ticket]
        if myticket not in self._download_tasks:
            return
        self.DownloadComplete(myticket, status, content)

    def _get_source_proxy(self, sourceid):
        return self._sources[sourceid]['proxy']

    def _get_source_search(self, sourceid, sourceticket):
        return self._sources[sourceid]['search'][sourceticket]

    def _set_source_search(self, sourceid, sourceticket, value):
        if sourceticket in self._sources[sourceid]['search']:
            raise KeyError('ticket %d exists in source search tasks', sourceticket)
        self._sources[sourceid]['search'][sourceticket] = value

    def _del_source_search(self, sourceid, sourceticket):
        del self._sources[sourceid]['search'][sourceticket]

    def _get_source_download(self, sourceid, sourceticket):
        return self._sources[sourceid]['download'][sourceticket]

    def _set_source_download(self, sourceid, sourceticket, value):
        if sourceticket in self._sources[sourceid]['download']:
            raise KeyError('ticket %d exists in source download tasks', sourceticket)
        self._sources[sourceid]['download'][sourceticket] = value

    def _del_source_download(self, sourceid, sourceticket):
        del self._sources[sourceid]['download'][sourceticket]

    def _do_search(self, ticket):
        task = self._search_tasks[ticket]
        nextsource = None
        while len(task['sources']) > 0:
            if task['sources'][0] in self._sources:
                nextsource = task['sources'][0]
                break
            else:
                logging.warning('Source %s not exist' % task['sources'][0])
                task['sources'].pop(0)
        if nextsource is None:
            status = STATUS_SUCCESS if not task['failure'] else STATUS_FAILURE
            self.SearchComplete(ticket, STATUS_SUCCESS, [])
        else:
            newticket = self._get_source_proxy(nextsource).Search(task['metadata'])
            self._set_source_search(nextsource, newticket, ticket)
            task['ticket'] = newticket
            self.SearchStarted(ticket, nextsource, self._sources[nextsource]['name'])

    @dbus.service.signal(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         signature='iiaa{sv}')
    def SearchComplete(self, ticket, status, results):
        if ticket in self._search_tasks:
            del self._search_tasks[ticket]

    @dbus.service.signal(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         signature='iiay')
    def DownloadComplete(self, ticket, status, content):
        if ticket in self._download_tasks:
            del self._download_tasks[ticket]

    @dbus.service.signal(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         signature='iss')
    def SearchStarted(self, ticket, sourceid, sourcename):
        pass

    @dbus.service.method(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         in_signature='a{sv}as',
                         out_signature='i')
    def Search(self, metadata, sources):
        self._n_search_tickets += 1
        ticket = self._n_search_tickets
        task = {
            'metadata': metadata,
            'sources': [str(id) for id in sources],
            'ticket': None,
            'failure': None,    # See comments in search_complete_cb()
            }
        self._search_tasks[ticket] = task
        self._do_search(ticket)
        return ticket

    @dbus.service.method(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         in_signature='i',
                         out_signature='')
    def CancelSearch(self,ticket):
        if ticket not in self._search_tasks:
            return
        task = self._search_tasks[ticket]
        sourceticket = task['ticket']
        sourceid = task['sources'][0]
        self._get_source_proxy(sourceid).CancelSearch(sourceticket)

    @dbus.service.method(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         in_signature='sv',
                         out_signature='i')
    def Download(self, source_id, downloaddata):
        if source_id not in self._sources:
            return -1
        sourceticket = self._get_source_proxy(source_id).Download(downloaddata)
        if sourceticket < 0:
            return -1
        self._n_download_tickets += 1
        ticket = self._n_download_tickets
        self._download_tasks[ticket] = {
            'ticket': sourceticket,
            'source': source_id,
            }
        self._set_source_download(source_id, sourceticket, ticket)
        return ticket

    @dbus.service.method(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         in_signature='i',
                         out_signature='')
    def CancelDownload(self,ticket):
        if ticket not in self._download_tasks:
            return
        task = self._download_tasks[ticket]
        sourceticket = task['ticket']
        sourceid = task['source']
        self._get_source_proxy(sourceid).CancelDownload(sourceticket)

    @dbus.service.method(dbus_interface=osdlyrics.LYRIC_SOURCE_INTERFACE,
                         in_signature='',
                         out_signature='aa{sv}')
    def ListSources(self):
        sources = self._config.get_string_list('Download/download-engine')
        ret = [{'id': id, 'name': self._sources[id]['name'], 'enabled': True} for
               id in sources if id in self._sources]
        enabled = frozenset(sources)
        for id, v in self._sources.items():
            if id not in enabled:
                ret.append({'id': id, 'name': v['name'], 'enabled': False})
        return ret
