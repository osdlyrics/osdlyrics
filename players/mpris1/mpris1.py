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
# along with OSD Lyrics.  If not, see <http://www.gnu.org/licenses/>.
#

from contextlib import contextmanager
import logging

import dbus

from osdlyrics.player_proxy import (
    BasePlayer, BasePlayerProxy, PlayerInfo, CAPS_NEXT, CAPS_PAUSE, CAPS_PLAY,
    CAPS_PREV, CAPS_SEEK, REPEAT_ALL, REPEAT_NONE, REPEAT_TRACK, STATUS_PAUSED,
    STATUS_PLAYING, STATUS_STOPPED)
from osdlyrics.metadata import Metadata

PROXY_NAME = 'Mpris1'
MPRIS1_PREFIX = 'org.mpris.'
MPRIS1_IFACE = 'org.freedesktop.MediaPlayer'

MPRIS1_CAN_GO_NEXT           = 1 << 0,
MPRIS1_CAN_GO_PREV           = 1 << 1,
MPRIS1_CAN_PAUSE             = 1 << 2,
MPRIS1_CAN_PLAY              = 1 << 3,
MPRIS1_CAN_SEEK              = 1 << 4,
MPRIS1_CAN_PROVIDE_METADATA  = 1 << 5,
MPRIS1_CAN_HAS_TRACKLIST     = 1 << 6

MPRIS1_CAPS_MAP = {
    1 << 0: CAPS_NEXT,
    1 << 1: CAPS_PREV,
    1 << 2: CAPS_PAUSE,
    1 << 3: CAPS_PLAY,
    1 << 4: CAPS_SEEK,
    }

# The constants actually have the same values but that is the legacy of MPRIS1.
MPRIS1_STATUS_MAP = {
    0: STATUS_PLAYING,
    1: STATUS_PAUSED,
    2: STATUS_STOPPED,
}


def player_info_from_name(name):
    return PlayerInfo(name, icon=name)


class ProxyObject(BasePlayerProxy):
    """ The DBus object for MPRIS1 player proxy
    """

    def __init__(self):
        """
        """
        super(ProxyObject, self).__init__('Mpris1')

    def _get_player_from_bus_names(self, names):
        return [player_info_from_name(name[len(MPRIS1_PREFIX):]) for name in names
                if name.startswith(MPRIS1_PREFIX) and not name.startswith(MPRIS1_PREFIX + 'MediaPlayer2.')]

    def do_list_active_players(self):
        return self._get_player_from_bus_names(self.connection.list_names())

    def do_list_supported_players(self):
        return self.do_list_activatable_players()

    def do_list_activatable_players(self):
        players = self._get_player_from_bus_names(self.connection.list_activatable_names())
        return players

    def do_connect_player(self, player_name):
        player = Mpris1Player(self, player_name)
        return player


class Mpris1Player(BasePlayer):
    def __init__(self, proxy, player_name):
        super(Mpris1Player, self).__init__(proxy,
                                           player_name)
        self._signals = []
        self._name_watch = None
        self._status_tuple = None, None, None, None
        self._use_cached_status = False
        try:
            self._player = dbus.Interface(self.connection.get_object(MPRIS1_PREFIX + player_name,
                                                                     '/Player'),
                                          MPRIS1_IFACE)
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
        if len(name) > 0:
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
        super(Mpris1Player, self).disconnect()

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
        if repeat in [REPEAT_TRACK, REPEAT_ALL]:
            self._player.Repeat(True)
        else:
            self._player.Repeat(False)

    def get_status(self):
        status_tuple = (self._status_tuple if self._use_cached_status else
                        self._player.GetStatus())
        status, shuffle, repeat, loop = status_tuple
        return MPRIS1_STATUS_MAP.get(status, STATUS_STOPPED)

    def get_repeat(self):
        status_tuple = (self._status_tuple if self._use_cached_status else
                        self._player.GetStatus())
        status, shuffle, repeat, loop = status_tuple
        if repeat:
            return REPEAT_TRACK
        if loop:
            return REPEAT_TRACK
        return REPEAT_NONE

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
        for bit, cap in MPRIS1_CAPS_MAP.iteritems():
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
