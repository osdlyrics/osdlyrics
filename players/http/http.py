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

import datetime
import logging
import time

import glib

from osdlyrics.metadata import Metadata
from osdlyrics.player_proxy import (BasePlayer, BasePlayerProxy, PlayerInfo,
                                    STATUS_PAUSED, STATUS_STOPPED)
import osdlyrics.timer

import server

CONNECTION_TIMEOUT = 1000


class HttpPlayerProxy(BasePlayerProxy):
    def __init__(self):
        super(HttpPlayerProxy, self).__init__('Http')
        self._server = server.HttpServer(('', 7119),
                                         self)
        self._server_watch = glib.io_add_watch(self._server.fileno(),
                                               glib.IO_IN,
                                               self._handle_req)
        self._players = {}
        self._connection_timer = glib.timeout_add(CONNECTION_TIMEOUT,
                                                  self._check_connection)
        self._player_counter = 1

    def _handle_req(self, fd, event):
        logging.debug('new request %s, %s', fd, event)
        self._server.handle_request()
        return True

    def add_player(self, name, caps):
        if name in self._players:
            name = '%s%s' % (name, self._player_counter)
            self._player_counter = self._player_counter + 1
        self._players[name] = HttpPlayer(self, name, caps)
        return name

    def remove_player(self, name):
        try:
            del self._players[name]
        except:
            pass

    def get_player(self, name):
        return self._players[name]

    def do_list_active_players(self):
        ret = []
        for v in self._players.values():
            ret.append(PlayerInfo(v.name))
        return ret

    def do_list_supported_players(self):
        return []

    def do_list_activatable_players(self):
        return []

    def do_connect_player(self, name):
        if name in self._players:
            return self._players[name]
        return None

    def _check_connection(self):
        for player in self._players.values():
            player.check_connection()
        return True


class HttpPlayer(BasePlayer):

    def __init__(self, proxy, name, caps):
        super(HttpPlayer, self).__init__(proxy, name)
        self._status = STATUS_STOPPED
        self._caps = caps
        self._metadata = Metadata()
        self._last_ping = datetime.datetime.now()
        self._timer = osdlyrics.timer.Timer()
        self._cmds = []

    def _ping(self):
        self._last_ping = datetime.datetime.now()

    def check_connection(self):
        now = datetime.datetime.now()
        duration = now - self._last_ping
        if duration.total_seconds() * 1000 > CONNECTION_TIMEOUT * 2:
            logging.warning('%s connection timeout', self.name)
            self.disconnect()

    def disconnect(self):
        self.proxy.remove_player(self.name)
        BasePlayer.disconnect(self)

    def do_update_track(self, metadata):
        self._ping()
        self._metadata = metadata
        self.track_changed()
        self._timer.stop()

    def do_update_status(self, status):
        self._status = status
        if status == STATUS_STOPPED:
            self._timer.stop()
        elif status == STATUS_PAUSED:
            self._timer.pause()
        else:
            self._timer.play()
        self.status_changed()

    def do_update_position(self, pos):
        if self._timer.set_time(pos):
            self.position_changed()

    def get_metadata(self):
        return self._metadata

    def get_status(self):
        return self._status

    def get_position(self):
        return self._timer.time

    def get_caps(self):
        return self._caps

    def query(self, timestamp):
        self._ping()
        cmds = []
        i = 0
        for cmd in self._cmds:
            if cmd[0] >= timestamp:
                break
            i = i + 1
        if i > 0:
            self._cmds = self._cmds[i:]
        for cmd in self._cmds:
            cmds.append(cmd[1])
        return cmds, int(time.time() * 10)

    def play(self):
        self._add_cmd('play')

    def pause(self):
        self._add_cmd('pause')

    def prev(self):
        self._add_cmd('prev')

    def next(self):
        self._add_cmd('next')

    def stop(self):
        self._add_cmd('stop')

    def set_position(self, pos):
        self._add_cmd('seek', {'pos': pos})

    def _add_cmd(self, cmd, params={}):
        self._cmds.append((int(time.time() * 10),
                          {'cmd': cmd, 'params': params}))


if __name__ == '__main__':
    proxy = HttpPlayerProxy()
    proxy.run()
