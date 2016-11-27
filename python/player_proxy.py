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

import logging

import dbus
import dbus.service

from app import App
from consts import (MPRIS2_PLAYER_INTERFACE, PLAYER_PROXY_INTERFACE,
                    PLAYER_PROXY_OBJECT_PATH_PREFIX)
from dbusext.service import Object as DBusObject, property as dbus_property
import errors
import timer
import utils

class ConnectPlayerError(errors.BaseError):
    """
    Exception raised when BasePlayerProxy.do_connect_player() fails
    """
    def __init__(self, message):
        super(ConnectPlayerError, self).__init__(message)

class BasePlayerProxy(dbus.service.Object):
    """ Base class to create an application to provide player proxy support
    """

    def __init__(self, name):
        """

        Arguments:
        - `name`: The suffix of the bus name. The full bus name is
          `org.osdlyrics.PlayerProxy.` + name
        """
        self._app = App('PlayerProxy.' + name)
        super(BasePlayerProxy, self).__init__(
            conn=self._app.connection,
            object_path=PLAYER_PROXY_OBJECT_PATH_PREFIX + name)
        self._name = name
        self._connected_players = {}

    @property
    def name(self):
        return self._name

    def run(self):
        self._app.run()

    @dbus.service.method(dbus_interface=PLAYER_PROXY_INTERFACE,
                         in_signature='',
                         out_signature='aa{sv}')
    def ListActivePlayers(self):
        return [player.to_dict() for player in self.do_list_active_players()]

    @dbus.service.method(dbus_interface=PLAYER_PROXY_INTERFACE,
                         in_signature='',
                         out_signature='aa{sv}')
    def ListSupportedPlayers(self):
        return [player.to_dict() for player in self.do_list_supported_players()]

    @dbus.service.method(dbus_interface=PLAYER_PROXY_INTERFACE,
                         in_signature='',
                         out_signature='aa{sv}')
    def ListActivatablePlayers(self):
        return [player.to_dict() for player in
                self.do_list_activatable_players()]

    @dbus.service.method(dbus_interface=PLAYER_PROXY_INTERFACE,
                         in_signature='s',
                         out_signature='o')
    def ConnectPlayer(self, player_name):
        if self._connected_players.setdefault(player_name, None):
            return self._connected_players[player_name].object_path
        player = self.do_connect_player(player_name)
        if player and player.connected:
            player.set_disconnect_cb(self._player_lost_cb)
            self._connected_players[player_name] = player
            logging.info('Connected to %s' % player.object_path)
            return player.object_path
        else:
            raise ConnectPlayerError('%s cannot be connected' % player_name)

    @dbus.service.signal(dbus_interface=PLAYER_PROXY_INTERFACE,
                         signature='s')
    def PlayerLost(self, player_name):
        pass

    def _player_lost_cb(self, player):
        if player.name in self._connected_players:
            del self._connected_players[player.name]
            self.PlayerLost(player.name)

    def do_list_active_players(self):
        """
        Lists supported players that are aready running

        Returns an list of `PlayerInfo` objects.

        Derived classes must reimplement this method.
        """
        raise NotImplementedError()

    def do_list_supported_players(self):
        """
        Lists supported players.

        Returns an list of `PlayerInfo` objects.

        Derived classes must reimplement this method.
        """
        raise NotImplementedError()

    def do_list_activatable_players(self):
        """
        Lists supported players installed on the system.

        Returns an list of `PlayerInfo` objects.

        Derived classes must reimplement this method.
        """
        raise NotImplementedError()

    def do_connect_player(self, playername):
        """
        Creates an Player object according to playername.

        Returns the created `BasePlayer` object, or None if cannot connect to
        the player with `playername`.
        """
        raise NotImplementedError()

class PlayerInfo(object):
    """Information about a supported player
    """

    def __init__(self, name, appname='', binname='', cmd='', icon=''):
        """
        """
        self._name = name
        self._appname = appname
        self._binname = binname
        self._cmd = cmd
        self._icon = icon

    @property
    def name(self):
        return self._name

    @property
    def appname(self):
        return self._appname

    @property
    def binname(self):
        return self._binname

    @property
    def cmd(self):
        return self._cmd

    @property
    def icon(self):
        return self._icon

    def to_dict(self):
        """
        Converts the PlayerInfo object to an dict that fits the specification
        """
        keys = ['name', 'appname', 'binname', 'cmd', 'icon']
        ret = {}
        for k in keys:
            ret[k] = utils.ensure_unicode(getattr(self, '_' + k))
        return ret

CAPS_NEXT = 1 << 0
CAPS_PREV = 1 << 1
CAPS_PAUSE = 1 << 2
CAPS_PLAY = 1 << 3
CAPS_SEEK = 1 << 4
CAPS_PROVIDE_METADATA = 1 << 5

STATUS_PLAYING = 0
STATUS_PAUSED = 1
STATUS_STOPPED = 2

REPEAT_NONE = 0
REPEAT_TRACK = 1
REPEAT_ALL = 2


class BasePlayer(DBusObject):
    """ Base class of a player

    Derived classes MUST reimplement following methods:

    - `get_metadata`
    - `get_position`
    - `get_caps`

    Derived classes SHOULD reimplement following methods if supported.
    - `get_status`
    - `get_repeat`
    - `set_repeat`
    - `get_shuffle`
    - `set_shuffle`
    - `play`
    - `pause`
    - `prev`
    - `next`
    - `set_position`
    - `set_volume`
    - `get_volume`
    """

    def __init__(self, proxy, name):
        """

        Arguments:
        - `proxy`: The BasePlayerProxy object that creates the player
        - `name`: The name of the player object
        """
        self.__object_path = PLAYER_PROXY_OBJECT_PATH_PREFIX + proxy.name + '/' + name
        super(BasePlayer, self).__init__(conn=proxy.connection,
                                         object_path=self.__object_path)
        self.__name = name
        self.__proxy = proxy
        self.__disconnect_cb = None
        self.__connected = True
        self.__timer = None
        self.__status = None
        self.__loop_status = None
        self.__metadata = None
        self.__current_trackid = 0
        self.__caps = None
        self.__shuffle = None

    def set_disconnect_cb(self, disconnect_cb):
        self.__disconnect_cb = disconnect_cb

    @property
    def name(self):
        return self.__name

    @property
    def proxy(self):
        return self.__proxy

    def disconnect(self):
        if self.__connected:
            self.__connected = False
            self.remove_from_connection()
            if callable(self.__disconnect_cb):
                self.__disconnect_cb(self)

    def get_status(self):
        """
        Return the playing status.

        The return value should be one of STATUS_PLAYING, STATUS_PAUSED, and
        STATUS_STOPPED

        Derived classes that supports playing status should reimplement this.
        """
        raise NotImplementedError()

    def get_metadata(self):
        """
        Return metadata of current track. The return value is of the type Metadata

        Derived classes must reimplement this method.
        """
        raise NotImplementedError()

    def get_position(self):
        """
        Gets the ellapsed time in current track.

        Returns the time in milliseconds.
        """
        raise NotImplementedError()

    def get_caps(self):
        """
        Return capablities of the players.

        The return value should be a set of CAPS_PLAY, CAPS_PAUSE, CAPS_NEXT,
        CAPS_PREV, CAPS_SEEK
        """
        raise NotImplementedError()

    def get_repeat(self):
        """
        Gets the repeat mode of the player

        Returns one of REPEAT_NONE, REPEAT_TRACK, or REPEAT_ALL

        The default implementation returns REPEAT_NONE
        """
        return REPEAT_NONE

    def set_repeat(self, mode):
        """
        Sets the repeat mode of the player

        Arguments:
        - `mode`: REPEAT_NONE, REPEAT_TRACK, or REPEAT_ALL
        """
        raise NotImplementedError()

    def get_shuffle(self):
        """
        Gets the shuffle mode of the player

        Returns True if the playlist is shuffle, or False otherwise.

        The default implementation returns False
        """
        return False

    def set_shuffle(self, shuffle):
        """
        Set whether the tracks in track list should be played randomly.

        Arguments:
        - `shuffle`: boolean, True if shuffle
        """
        raise NotImplementedError()

    def play(self):
        """
        Start/continue playing the current track
        """
        raise NotImplementedError()

    def pause(self):
        """
        Pause the current track
        """
        raise NotImplementedError()

    def stop(self):
        """
        Stop playing.
        """
        raise NotImplementedError()

    def prev(self):
        """
        Play the previous track.
        """
        raise NotImplementedError()

    def next(self):
        """
        Play the next track.
        """
        raise NotImplementedError()

    def set_position(self, pos):
        """
        Seek to the given position.

        Arguments:
        - `pos`: Seek time in millisecond
        """
        raise NotImplementedError()

    def get_volume(self):
        """
        Gets the volume of the player.

        Return the volume in the range of [0.0, 1.0]
        """
        raise NotImplementedError()

    def set_volume(self, volume):
        """
        Sets the volume of the player.

        Arguments:
        - `volume`: volume in the range of [0.0, 1.0]
        """
        raise NotImplementedError()

    def _setup_timer_status(self, status):
        status_map = {
            STATUS_PAUSED: 'pause',
            STATUS_PLAYING: 'play',
            STATUS_STOPPED: 'stop',
            }
        if self.__timer:
            getattr(self.__timer, status_map[status])()

    def _setup_timer(self):
        if self.__timer is None:
            self.__timer = timer.Timer()
            self._setup_timer_status(self._get_cached_status())

    def _get_cached_position(self):
        """
        Get the current position from cached timer if possible
        """
        if self.__timer is None:
            self._setup_timer()
            if self._get_cached_status() != STATUS_STOPPED:
                self.__timer.time = self.get_position()
        return self.__timer.time

    def _get_cached_loop_status(self):
        if self.__loop_status is None:
            self.__loop_status = self.get_repeat()
        return self.__loop_status

    def _get_cached_status(self):
        if self.__status is None:
            self.__status = self.get_status()
        return self.__status

    def _get_cached_metadata(self):
        if self.__metadata is None:
            self.__metadata = self._make_metadata(self.get_metadata())
        return self.__metadata

    def _make_metadata(self, metadata):
        dct = metadata.to_mpris2()
        dct['mpris:trackid'] = self._get_current_trackid()
        return dct

    def _get_current_trackid(self):
        return '/%s' % self.__current_trackid

    def _get_cached_caps(self):
        if self.__caps is None:
            self.__caps = self.get_caps()
        return self.__caps

    def _get_cached_shuffle(self):
        if self.__shuffle is None:
            self.__shuffle = self.get_shuffle()
        return self.__shuffle

    @property
    def connected(self):
        return self.__connected

    @property
    def object_path(self):
        return self.__object_path

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Next(self):
        self.next()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Previous(self):
        self.prev()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Pause(self):
        self.pause()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Stop(self):
        self.stop()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Play(self):
        self.play()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='x',
                         out_signature='')
    def Seek(self, offset):
        pos = self._get_cached_position()
        pos += offset / 1000
        if pos < 0:
            pos = 0
        self.set_position(pos)

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='ox',
                         out_signature='')
    def SetPosition(self, trackid, position):
        if trackid != self._get_current_trackid():
            return
        self.set_position(position / 1000)

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def PlayPause(self):
        if hasattr(self, 'play_pause'):
            self.play_pause()
        else:
            status = self._get_cached_status()
            if status == STATUS_PLAYING:
                self.pause()
            else:
                self.play()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='s',
                         out_signature='')
    def OpenUri(self, uri):
        self.open_uri(uri)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='s',
                   writeable=False)
    def PlaybackStatus(self):
        status_map = {
            STATUS_PLAYING: 'Playing',
            STATUS_PAUSED: 'Paused',
            STATUS_STOPPED: 'Stopped',
            }
        return status_map[self._get_cached_status()]

    @PlaybackStatus.setter
    def PlaybackStatus(self, status):
        self.__status = status

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='s')
    def LoopStatus(self):
        status_map = {
            REPEAT_NONE: 'None',
            REPEAT_ALL: 'Playlist',
            REPEAT_TRACK: 'Track',
            }
        return status_map[self._get_cached_loop_status()]

    @LoopStatus.setter
    def LoopStatus(self, loop_status):
        self.__loop_status = loop_status

    @LoopStatus.dbus_setter
    def LoopStatus(self, loop_status):
        status_map = {
            'None': REPEAT_NONE,
            'Playlist': REPEAT_ALL,
            'Track': REPEAT_TRACK,
            }
        if loop_status not in status_map:
            raise ValueError('Unknown loop status ' + loop_status)
        self.set_repeat(status_map[loop_status])

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def Rate(self):
        return 1.0

    @Rate.setter
    def Rate(self, rate):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b')
    def Shuffle(self):
        return self._get_cached_shuffle()

    @Shuffle.setter
    def Shuffle(self, shuffle):
        self.__shuffle = shuffle

    @Shuffle.dbus_setter
    def Shuffle(self, shuffle):
        self.set_shuffle(shuffle)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='a{sv}',
                   writeable=False)
    def Metadata(self):
        return self._get_cached_metadata()

    @Metadata.setter
    def Metadata(self, metadata):
        self.__metadata = metadata

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def Volume(self):
        return self.get_volume()

    @Volume.dbus_setter
    def Volume(self, volume):
        if volume < 0.0:
            volume = 0.0
        if volume > 1.0:
            volume = 1.0
        self.set_volume(volume)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='x')
    def Position(self):
        return self._get_cached_position()

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def MinimumRate(self):
        return 1.0

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def MaximumRate(self):
        return 1.0

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanGoNext(self):
        return CAPS_NEXT in self._get_cached_caps()

    @CanGoNext.setter
    def CanGoNext(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanGoPrevious(self):
        return CAPS_PREV in self._get_cached_caps()

    @CanGoPrevious.setter
    def CanGoPrevious(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanPlay(self):
        return CAPS_PLAY in self._get_cached_caps()

    @CanPlay.setter
    def CanPlay(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanPause(self):
        return CAPS_PAUSE in self._get_cached_caps()

    @CanPause.setter
    def CanPause(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanSeek(self):
        return CAPS_SEEK in self._get_cached_caps()

    @CanSeek.setter
    def CanSeek(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b')
    def CanControl(self):
        return True

    @dbus.service.signal(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         signature='x')
    def Seeked(self, position):
        pass

    def track_changed(self, metadata=None):
        self.__current_trackid += 1
        if self.__timer is not None:
            self.__timer.time = self.get_position()
        if metadata is None:
            metadata = self.get_metadata()
        self.Metadata = self._make_metadata(metadata)

    def status_changed(self):
        """
        Notify that the playing status has been changed.
        """
        status = self.get_status()
        self._setup_timer_status(status)
        self.PlaybackStatus = status

    def repeat_changed(self):
        """
        Notify the repeat mode has been changed.
        """
        self.LoopStatus = self.get_repeat()

    def shuffle_changed(self):
        """
        Notify the shuffle mode has been changed.
        """
        self.Shuffle = self.get_shuffle()

    def caps_changed(self):
        """
        Notify the capability of the player has been changed.
        """
        orig_caps = self.__caps
        self.__caps = self.get_caps()
        if orig_caps is not None:
            caps_map = {
                CAPS_NEXT: 'CanGoNext',
                CAPS_PREV: 'CanGoPrevious',
                CAPS_PLAY: 'CanPlay',
                CAPS_PAUSE: 'CanPause',
                CAPS_SEEK: 'CanSeek',
                }
            for cap, method in caps_map.items():
                if cap in orig_caps != cap in self.__caps:
                    setattr(self, method, cap in self.__caps)

    def position_changed(self):
        """
        Notify that the position has been changed
        """
        if self.__timer is not None:
            self.__timer.time = self.get_position()
        self.Seeked(self.__timer.time * 1000)
