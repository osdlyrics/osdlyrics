# -*- coding: utf-8 -*-
#
# Copyright (C) 2012  Tiger Soldier
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
from contextlib import contextmanager
import logging

import dbus

from osdlyrics.metadata import Metadata
from osdlyrics.player_proxy import (CAPS, REPEAT, STATUS, BasePlayer,
                                    BasePlayerProxy, PlayerInfo)

MPRIS1_INTERFACE = 'org.freedesktop.MediaPlayer'
MPRIS1_PREFIX = 'org.mpris.'

# These constants map flags/enums from MPRIS1-specific values to OSDLyrics values.
CAPS_MAP = {
    1 << 0: CAPS.NEXT,
    1 << 1: CAPS.PREV,
    1 << 2: CAPS.PAUSE,
    1 << 3: CAPS.PLAY,
    1 << 4: CAPS.SEEK,
}
STATUS_MAP = {
    0: STATUS.PLAYING,
    1: STATUS.PAUSED,
    2: STATUS.STOPPED,
}


class ProxyObject(BasePlayerProxy):
    """ The DBus object for MPRIS1 player proxy
    """

    def __init__(self):
        """
        """
        super().__init__('Mpris1')

    def _get_player_from_bus_names(self, names):
        return [
            PlayerInfo.from_name(name[len(MPRIS1_PREFIX):])
            for name in names
            if name.startswith(MPRIS1_PREFIX) and not name.startswith(MPRIS1_PREFIX + 'MediaPlayer2.')
        ]

    def do_list_active_players(self):
        return self._get_player_from_bus_names(map(str, self.connection.list_names()))

    def do_list_supported_players(self):
        return self.do_list_activatable_players()

    def do_list_activatable_players(self):
        players = self._get_player_from_bus_names(map(str, self.connection.list_activatable_names()))
        return players

    def do_connect_player(self, player_name):
        player = Mpris1Player(self, player_name)
        return player


class Mpris1Player(BasePlayer):
    def __init__(self, proxy, player_name):
        super().__init__(proxy, player_name)
        self._signals = []
        self._name_watch = None
        self._status_tuple = None, None, None, None
        self._use_cached_status = False
        try:
            self._player = dbus.Interface(self.connection.get_object(MPRIS1_PREFIX + player_name,
                                                                     '/Player'),
                                          MPRIS1_INTERFACE)
            mpris1_service_name = MPRIS1_PREFIX + player_name
            self._signals.append(self._player.connect_to_signal('TrackChange',
                                                                self._track_change_cb))
            self._signals.append(self._player.connect_to_signal('StatusChange',
                                                                self._status_change_cb))
            self._signals.append(self._player.connect_to_signal('CapsChange',
                                                                self._caps_change_cb))
            self._name_watch = self.connection.watch_name_owner(mpris1_service_name,
                                                                self._name_lost)
        except Exception as e:
            logging.error('Fail to connect to mpris1 player %s: %s', player_name, e)
            self.disconnect()

    def _name_lost(self, name):
        if name:
            return
        self.disconnect()

    def disconnect(self):
        for handler in self._signals:
            handler.remove()
        self._signals = []
        if self._name_watch:
            self._name_watch.cancel()
            self._name_watch = None
        self._player = None
        super().disconnect()

    def next(self):
        self._player.Next()

    def prev(self):
        self._player.Prev()

    def pause(self):
        self._player.Pause()

    def stop(self):
        self._player.Stop()

    def play(self):
        self._player.Play()

    def set_repeat(self, repeat):
        if repeat in (REPEAT.TRACK, REPEAT.ALL):
            self._player.Repeat(True)
        else:
            self._player.Repeat(False)

    def get_status(self):
        status_tuple = (self._status_tuple if self._use_cached_status else
                        self._player.GetStatus())
        status, shuffle, repeat, loop = status_tuple
        return STATUS_MAP.get(status, STATUS.STOPPED)

    def get_repeat(self):
        status_tuple = (self._status_tuple if self._use_cached_status else
                        self._player.GetStatus())
        status, shuffle, repeat, loop = status_tuple
        if repeat:
            return REPEAT.TRACK
        if loop:
            return REPEAT.TRACK
        return REPEAT.NONE

    def get_shuffle(self):
        status_tuple = (self._status_tuple if self._use_cached_status else
                        self._player.GetStatus())
        status, shuffle, repeat, loop = status_tuple
        return True if shuffle == 1 else False

    def get_metadata(self):
        mt = self._player.GetMetadata()
        logging.debug(repr(mt))
        return Metadata.from_dict(mt)

    def get_caps(self):
        caps = set()
        mpris1_caps = self._player.GetCaps()
        for bit, cap in CAPS_MAP.items():
            if mpris1_caps & bit:
                caps.add(cap)
        return caps

    def set_volume(self, volume):
        volume = int(volume * 100)
        if volume < 0:
            volume = 0
        if volume > 100:
            volume = 100
        self._player.VolumeSet(volume)

    def get_volume(self):
        volume = float(self._player.VolumeGet()) / 100
        if volume > 1.0:
            volume = 1.0
        if volume < 0.0:
            volume = 0.0
        return volume

    def set_position(self, time_in_mili):
        self._player.PositionSet(time_in_mili)

    def get_position(self):
        return self._player.PositionGet()

    def _track_change_cb(self, metadata):
        metadata = Metadata.from_dict(metadata)
        self.track_changed(metadata)

    def _status_change_cb(self, status_tuple):
        status, shuffle, repeat, loop = status_tuple
        old_status, old_shuffle, old_repeat, old_loop = self._status_tuple
        self._status_tuple = status_tuple
        with self._reuse_cached_status_tuple():
            if status != old_status:
                self.status_changed()
            if repeat != old_repeat or loop != old_loop:
                self.repeat_changed()
            if shuffle != old_shuffle:
                self.shuffle_changed()

    def _caps_change_cb(self, caps):
        self.caps_changed()

    @contextmanager
    def _reuse_cached_status_tuple(self):
        try:
            self._use_cached_status = True
            yield
        finally:
            self._use_cached_status = False


def run():
    mpris1 = ProxyObject()
    mpris1.run()


if __name__ == '__main__':
    run()
