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

import dbus.service
import glib

import osdlyrics
from osdlyrics.app import App
from osdlyrics.consts import (MPRIS2_PLAYER_INTERFACE, MPRIS2_ROOT_INTERFACE,
                              MPRIS2_OBJECT_PATH)
from osdlyrics.dbusext.service import (Object as DBusObject,
                                       property as dbus_property)
import osdlyrics.timer

import config


class PlayerSupport(dbus.service.Object):
    """ Implement org.osdlyrics.Player Interface
    """

    DETECT_PLAYER_TIMEOUT = 1000

    def __init__(self, conn):
        """
        Arguments:
         - `conn`: DBus connection of the object
        """
        dbus.service.Object.__init__(self,
                                     conn=conn,
                                     object_path=osdlyrics.PLAYER_OBJECT_PATH)
        self._active_player = None
        self._player_proxies = {}
        self._connect_player_proxies()
        self._start_detect_player()
        self._mpris2_player = Mpris2Player(conn)

    def _start_detect_player(self):
        self._detect_timer = glib.timeout_add(self.DETECT_PLAYER_TIMEOUT,
                                              lambda : not self._detect_player())

    def _detect_player(self):
        """
        Detects active players.

        This is a callback function of a timer. If active player detected, try to
        connect to the player and remove the timer.
        """
        detected = False
        for proxy in self._player_proxies.values():
            try:
                active_players = proxy.ListActivePlayers()
                for player_info in active_players:
                    if self._connect_player(proxy, player_info):
                        detected = True
                        break
                if detected:
                    break
            except:
                pass
        if detected and self._detect_timer:
            glib.source_remove(self._detect_timer)
            self._detect_timer = None
        return detected

    def _connect_proxy(self, bus_name, activate):
        if not bus_name.startswith(osdlyrics.PLAYER_PROXY_BUS_NAME_PREFIX):
            return
        logging.info('Connecting to player proxy %s', bus_name)
        proxy_name = bus_name[len(osdlyrics.PLAYER_PROXY_BUS_NAME_PREFIX):]
        if activate:
            try:
                self.connection.activate_name_owner(bus_name)
            except Exception as e:
                logging.warning('Cannot activate proxy %s: %s' % (bus_name, e))
        self.connection.watch_name_owner(bus_name,
                                         lambda name: self._proxy_name_changed(proxy_name, len(name) == 0))

    def _connect_player_proxies(self):
        """
        Activates all player proxy services
        """
        active_names = self.connection.list_names()
        for bus_name in active_names:
            self._connect_proxy(bus_name, False)
        activatable_names = self.connection.list_activatable_names()
        for bus_name in activatable_names:
            self._connect_proxy(bus_name, True)

    def _connect_player(self, proxy, player_info):
        """
        Tries to connect player through proxy

        Return True if connect successful
        """
        try:
            path = proxy.ConnectPlayer(player_info['name'])
            player = self.connection.get_object(proxy.bus_name,
                                                path)
            self._active_player = {'info': player_info,
                                   'player': player,
                                   'proxy': proxy}
            self._mpris2_player.connect_player(player)
            self.PlayerConnected(player_info)
            return True
        except Exception:
            return False

    def _player_lost_cb(self, player_name):
        if self._active_player and self._active_player['info']['name'] == player_name:
            logging.info('Player %s lost', player_name)
            self._active_player = None
            self._mpris2_player.disconnect_player()
            self.PlayerLost()
            self._start_detect_player()

    def _proxy_name_changed(self, proxy_name, lost):
        bus_name = osdlyrics.PLAYER_PROXY_BUS_NAME_PREFIX + proxy_name
        if not lost:
            logging.info('Get player proxy %s' % proxy_name)
            proxy = self.connection.get_object(bus_name,
                                               osdlyrics.PLAYER_PROXY_OBJECT_PATH_PREFIX + proxy_name)
            proxy.connect_to_signal('PlayerLost',
                                    self._player_lost_cb)
            self._player_proxies[proxy_name] = dbus.Interface(proxy, osdlyrics.PLAYER_PROXY_INTERFACE)
        else:
            if not proxy_name in self._player_proxies:
                return
            logging.info('Player proxy %s lost' % proxy_name)
            proxy = self._player_proxies[proxy_name]
            # If current player is provided by the proxy, it is lost.
            if self._active_player and self._active_player['proxy'] == proxy:
                self._player_lost_cb(self._active_player['info']['name'])
                del self._player_proxies[proxy_name]
            # Try to reactivate proxy
            try:
                self.connection.activate_name_owner(bus_name)
            except:
                pass

    @dbus.service.method(dbus_interface=osdlyrics.PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='aa{sv}')
    def ListSupportedPlayers(self):
        ret = []
        for proxy in self._player_proxies.values():
            try:
                ret = ret + proxy.ListSupportedPlayers()
            except:
                pass
        return ret

    @dbus.service.method(dbus_interface=osdlyrics.PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='aa{sv}')
    def ListActivatablePlayers(self):
        ret = []
        for proxy in self._player_proxies.values():
            try:
                ret = ret + proxy.ListActivatablePlayers()
            except:
                pass
        return ret

    @dbus.service.method(dbus_interface=osdlyrics.PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='ba{sv}')
    def GetCurrentPlayer(self):
        if not self._active_player and not self._detect_player():
            return False, {}
        return True, self._active_player['info']

    @dbus.service.signal(dbus_interface=osdlyrics.PLAYER_INTERFACE,
                         signature='')
    def PlayerLost(self):
        pass

    @dbus.service.signal(dbus_interface=osdlyrics.PLAYER_INTERFACE,
                         signature='a{sv}')
    def PlayerConnected(self, player_info):
        pass

    @property
    def current_player(self):
        return self._mpris2_player


class Mpris2Player(DBusObject):

    def __init__(self, conn):
        super(Mpris2Player, self).__init__(conn=conn,
                                           object_path=MPRIS2_OBJECT_PATH)
        self._signals = []
        self._player = None
        self._timer = osdlyrics.timer.Timer()
        self._clear_properties()

    def _clear_properties(self):
        self.LoopStatus = 'None'
        self.PlaybackStatus = 'Stopped'
        self.Metadata = dbus.Dictionary(signature='sv')
        self.Shuffle = False
        self._timer.stop()
        self._timer.time = 0

    def connect_player(self, player_proxy):
        if self._player == player_proxy:
            return
        if self._player is not None:
            self.disconnect_player()
        self._player = player_proxy
        self._signals = []
        self._signals.append(self._player.connect_to_signal('Seeked',
                                                            self._seeked_cb))
        self._signals.append(self._player.connect_to_signal('PropertiesChanged',
                                                            self._properties_changed_cb))
        self.PlaybackStatus = self._player.Get(MPRIS2_PLAYER_INTERFACE, 'PlaybackStatus')
        self.LoopStatus = self._player.Get(MPRIS2_PLAYER_INTERFACE, 'LoopStatus')
        self.Shuffle = self._player.Get(MPRIS2_PLAYER_INTERFACE, 'Shuffle')
        self.Metadata = self._player.Get(MPRIS2_PLAYER_INTERFACE, 'Metadata')
        self._setup_timer_status(self._playback_status)
        self._timer.time = self._player.Get(MPRIS2_PLAYER_INTERFACE, 'Position')

    def disconnect_player(self):
        for signal in self._signals:
            signal.remove()
        self._signals = []
        del self._player
        self._player = None
        self._clear_properties()

    def _setup_timer_status(self, status):
        status_map = {
            'Playing': 'play',
            'Paused': 'pause',
            'Stopped': 'stop',
            }
        if status in status_map:
            getattr(self._timer, status_map[status])()

    def _seeked_cb(self, position):
        self._timer.time = position / 1000
        self.Seeked(position)

    def _properties_changed_cb(self, iface, changed, invalidated):
        accepted_properties = set(['PlaybackStatus',
                                   'LoopStatus',
                                   'Rate',
                                   'Shuffle',
                                   'Metadata',
                                   'Volume',
                                   'MinimumRate',
                                   'MaximumRate',
                                   'CanGoNext',
                                   'CanGoPrevious',
                                   'CanPlay',
                                   'CanPause',
                                   'CanSeek',
                                   ])
        for k, v in changed.iteritems():
            if k in accepted_properties:
                setattr(self, k, v)

    ################################################
    # org.mpris.MediaPlayer2.Player interface
    ################################################

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Next(self):
        self._player.Next()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Previous(self):
        self._player.Previous()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Pause(self):
        self._player.Pause()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Stop(self):
        self._player.Stop()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Play(self):
        self._player.Play()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='x',
                         out_signature='')
    def Seek(self, offset):
        self._player.Seek(offset)

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='ox',
                         out_signature='')
    def SetPosition(self, trackid, position):
        self._player.SetPosition(trackid, position)

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='',
                         out_signature='')
    def PlayPause(self):
        self._player.PlayPause()

    @dbus.service.method(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         in_signature='s',
                         out_signature='')
    def OpenUri(self, uri):
        self._player.OpenUri(uri)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='s',
                   writeable=False)
    def PlaybackStatus(self):
        return self._playback_status

    @PlaybackStatus.setter
    def PlaybackStatus(self, status):
        self._playback_status = status
        self._setup_timer_status(status)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='s')
    def LoopStatus(self):
        return self._loop_status

    @LoopStatus.setter
    def LoopStatus(self, loop_status):
        self._loop_status = loop_status

    @LoopStatus.dbus_setter
    def LoopStatus(self, loop_status):
        self._player.Set(MPRIS2_PLAYER_INTERFACE, 'LoopStatus', loop_status)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def Rate(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'Rate')

    @Rate.setter
    def Rate(self, rate):
        pass

    @Rate.dbus_setter
    def Rate(self, rate):
        self._player.Set(MPRIS2_PLAYER_INTERFACE, 'Rate', rate)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b')
    def Shuffle(self):
        return self._shuffle

    @Shuffle.setter
    def Shuffle(self, shuffle):
        self._shuffle = shuffle

    @Shuffle.dbus_setter
    def Shuffle(self, shuffle):
        self._player.Set(MPRIS2_PLAYER_INTERFACE, 'Shuffle', shuffle)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='a{sv}',
                   writeable=False)
    def Metadata(self):
        return self._metadata

    @Metadata.setter
    def Metadata(self, metadata):
        self._metadata = metadata
        self._timer.time = 0 if not self._player else self._player.Get(
            MPRIS2_PLAYER_INTERFACE, 'Position')

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def Volume(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'Volume')

    @Volume.setter
    def Volume(self, volume):
        pass

    @Volume.dbus_setter
    def Volume(self, volume):
        return self._player.Set(MPRIS2_PLAYER_INTERFACE, 'Volume', volume)

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='x')
    def Position(self):
        return self._timer.time * 1000

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def MinimumRate(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'MinimumRate')

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='d')
    def MaximumRate(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'MaximumRate')

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanGoNext(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'CanGoNext')

    @CanGoNext.setter
    def CanGoNext(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanGoPrevious(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'CanGoPrevious')

    @CanGoPrevious.setter
    def CanGoPrevious(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanPlay(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'CanPlay')

    @CanPlay.setter
    def CanPlay(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanPause(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'CanPause')

    @CanPause.setter
    def CanPause(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b',
                   writeable=False)
    def CanSeek(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'CanSeek')

    @CanSeek.setter
    def CanSeek(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                   type_signature='b')
    def CanControl(self):
        return self._player.Get(MPRIS2_PLAYER_INTERFACE, 'CanControl')

    @dbus.service.signal(dbus_interface=MPRIS2_PLAYER_INTERFACE,
                         signature='x')
    def Seeked(self, position):
        pass

    ################################################
    # org.mpris.MediaPlayer2 interface
    ################################################

    @dbus.service.method(dbus_interface=MPRIS2_ROOT_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Raise(self):
        pass

    @dbus.service.method(dbus_interface=MPRIS2_ROOT_INTERFACE,
                         in_signature='',
                         out_signature='')
    def Quit(self):
        pass

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='b')
    def CanQuit(self):
        return False

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='b')
    def Fullscreen(self):
        return False

    @Fullscreen.dbus_setter
    def Fullscreen(self, value):
        pass

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='b')
    def CanSetFullscreen(self):
        return False

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='b')
    def CanRaise(self):
        return False

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='b')
    def HasTrackList(self):
        return False

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='s')
    def Identity(self):
        return config.PROGRAM_NAME

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='s')
    def DesktopEntry(self):
        return 'osdlyrics'

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='as')
    def SupportedUriSchemes(self):
        return dbus.Array(signature='s')

    @dbus_property(dbus_interface=MPRIS2_ROOT_INTERFACE,
                   type_signature='as')
    def SupportedMimeTypes(self):
        return dbus.Array(signature='s')


def test():
    app = App('osdlyrics')
    mpris2_name = dbus.service.BusName('org.mpris.osdlyrics', app.connection)
    player_support = PlayerSupport(app.connection)
    app.run()


if __name__ == '__main__':
    test()
