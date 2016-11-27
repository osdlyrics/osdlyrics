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
#/

"""MPD support for OSD Lyrics. Requires MPD >= 0.16 and mpd-python >= 0.3
"""

import __builtin__
import logging
import os
import select

import dbus
import dbus.service
import glib
import gobject
try:
    import mpd
except ImportError:
    mpd = None

import osdlyrics
import osdlyrics.consts
from osdlyrics.metadata import Metadata
from osdlyrics.player_proxy import (
    BasePlayer, BasePlayerProxy, PlayerInfo, CAPS_NEXT, CAPS_PAUSE, CAPS_PLAY,
    CAPS_PREV, CAPS_SEEK, REPEAT_ALL, REPEAT_NONE, REPEAT_TRACK, STATUS_PAUSED,
    STATUS_PLAYING, STATUS_STOPPED)
from osdlyrics.timer import Timer
from osdlyrics.utils import cmd_exists

if mpd is None or not hasattr(mpd.MPDClient(), 'send_idle'):
    logging.error('python-mpd >= 0.3 is required.')
    exit(1)

PLAYER_NAME = 'Mpd'
DEFAULT_HOST = 'localhost'
DEFAULT_PORT = 6600

class NoConnectionError(Exception):
    pass

class CommandCallback:
    def __init__(self, command, callback):
        self.command = command
        self.callback = callback

    def call(self, *args):
        if callable(self.callback):
            self.callback(*args)

class Cmds:
    CONFIG = 'config'
    CURRENTSONG = 'currentsong'
    IDLE = 'idle'
    NEXT = 'next'
    NOIDLE = 'noidle'
    PAUSE = 'pause'
    PLAY = 'play'
    PREVIOUS = 'previous'
    RANDOM = 'random'
    REPEAT = 'repeat'
    REPLAY_GAIN_MODE = 'replay_gain_mode'
    REPLAY_GAIN_STATUS = 'replay_gain_status'
    SEEK = 'seek'
    SEEKCUR = 'seekcur'
    SEEKID = 'seekid'
    SETVOL = 'setvol'
    SINGLE = 'single'
    STATUS = 'status'
    STOP = 'stop'

class MpdProxy(BasePlayerProxy):
    def __init__(self):
        super(MpdProxy, self).__init__('Mpd')
        self._player = None
        self._init_address()
        self._client = None
        self._player_info = PlayerInfo(name=PLAYER_NAME,
                                       appname='mpd',
                                       binname='mpd',
                                       cmd='mpd')
        self._player = None
        self._io_watch = None
        self._fetch_queue = []
        self._on_idle = False

    def _init_address(self):
        """
        Initialize the host and port of MPD daemon
        """
        if 'MPD_HOST' in os.environ:
            self._host = os.environ['MPD_HOST']
        else:
            self._host = DEFAULT_HOST
        if 'MPD_PORT' in os.environ and os.environ['MPD_PORT'].isdigit():
            self._port = os.environ['MPD_PORT']
        else:
            self._port = DEFAULT_PORT

    def _connect_mpd(self):
        if self._is_connected():
            return True
        if not self._client:
            self._client = mpd.MPDClient()
        try:
            self._client.connect(self._host, self._port)
        except IOError as e:
            logging.info("Could not connect to '%s': %s", self._host,
                         e.strerror)
            return False
        except mpd.MPDError as e:
            logging.info("Could not connect to '%s': %s", self._host, e)
            return False
        self._io_watch = gobject.io_add_watch(self._client,
                                              glib.IO_IN,
                                              self._on_data)
        return True

    def do_list_active_players(self):
        if self._connect_mpd():
            return [self._player_info]
        else:
            return []

    def do_list_supported_players(self):
        return [self._player_info]

    def do_list_activatable_players(self):
        if cmd_exists('mpd'):
            return [self._player_info]
        else:
            return []

    def do_connect_player(self, playername):
        if playername != PLAYER_NAME:
            return None
        if self._player:
            return self._player
        if not self._connect_mpd():
            return None
        self._player = MpdPlayer(self, playername)
        self._start_idle()
        return self._player

    def _on_data(self, client, condition):
        while len(self._fetch_queue) > 0:
            try:
                cmd_item = self._fetch_queue.pop(0)
                logging.debug('fetch cmd: %s', cmd_item.command)
                if not callable(cmd_item.callback):
                    continue
                logging.debug('client pending: %s', self._client._pending)
                retval = getattr(self._client, 'fetch_' + cmd_item.command)()
                cmd_item.call(retval)
            except Exception as e:
               logging.exception(e)
               self._on_disconnect()
            return True

        # no pending data, socket might be closed
        data = os.read(self._client.fileno(), 1024)
        if len(data) == 0:              # connection closed
            logging.info('connection closed')
            self._on_disconnect()
            return
        else:
            raise RuntimeError('Unexpected data: %s', data)

    def _on_disconnect(self):
        if self._io_watch:
            glib.remove_source(self._io_watch)
            self._io_watch = None
            self._client.disconnect()
            self._player.disconnect()
            self._player = None
        self._fetch_queue = []
        self._on_idle = False

    def _is_connected(self):
        return True if self._io_watch else False

    def send_command(self, command, callback, *args):
        if not self._is_connected:
            raise NoConnectionError()
        on_idle = self._is_on_idle()
        logging.debug('send %s %s', command, args)
        logging.debug('on idle: %s', on_idle)
        if on_idle:
            self._stop_idle()
        getattr(self._client, 'send_' + command)(*args)
        if callable(callback):
            self._enqueue_callback(command, callback)
        if on_idle:
            self._start_idle()
        logging.debug('cmd queue: %s',
                      [item.command for item in self._fetch_queue])

    def send_command_sync(self, command, callback, *args):
        self.send_command(command, callback, *args)
        logging.debug('send sync to fetch: %s', self._fetch_queue)
        while len(self._fetch_queue) > 1 \
                or (len(self._fetch_queue) == 1
                    and self._fetch_queue[0].command != Cmds.IDLE):
            select.select((self._client,), (), ())
            self._on_data(self._client, glib.IO_IN)

    def _enqueue_callback(self, command, callback):
        self._fetch_queue.append(CommandCallback(command, callback))

    def _is_on_idle(self):
        return len(self._fetch_queue) > 0 \
            and self._fetch_queue[-1].command == Cmds.IDLE

    def _start_idle(self):
        if not self._is_connected() or self._is_on_idle():
            return
        logging.debug('start idle')
        self.send_command(Cmds.IDLE, self._fetch_idle)

    def _stop_idle(self):
        if not self._is_connected() or not self._is_on_idle():
            return
        logging.debug('stop idle')
        self._client.send_noidle()
        self._enqueue_callback(Cmds.NOIDLE, None)

    def _fetch_idle(self, changes):
        if self._player:
            self._player.handle_changes(changes)
        self._start_idle()

    @dbus.service.method(in_signature='',
                         out_signature='a{sv}',
                         dbus_interface=osdlyrics.consts.PLAYER_PROXY_INTERFACE)
    def DebugInfo(self):
        ret = {}
        ret['host'] = self._host
        ret['port'] = dbus.UInt32(self._port)
        ret['connected'] = dbus.Boolean(self._is_connected())
        ret['on_idle'] = dbus.Boolean(self._is_on_idle())
        ret['fetch_queue'] = \
            '[' + ','.join([item.command for item in self._fetch_queue]) + ']'
        if self._player:
            ret['player'] = self._player.debug_info()
        return ret

class MpdPlayer(BasePlayer):

    CMD_HANDLERS = {
        Cmds.CURRENTSONG: '_handle_currentsong',
        Cmds.NEXT: None,
        Cmds.PAUSE: None,
        Cmds.PLAY: None,
        Cmds.PREVIOUS: None,
        Cmds.RANDOM: None,
        Cmds.REPEAT: None,
        Cmds.REPLAY_GAIN_MODE: None,
        Cmds.REPLAY_GAIN_STATUS: '_handle_replay_gain_status',
        Cmds.SEEK: None,
        Cmds.SEEKCUR: None,
        Cmds.SEEKID: None,
        Cmds.SETVOL: None,
        Cmds.SINGLE: None,
        Cmds.STATUS: '_handle_status',
        Cmds.STOP: None,
    }

    CHANGE_CMDS = {
        'player': [Cmds.STATUS],
        'options': [Cmds.STATUS],
    }

    STATUS_CHANGE_MAP = {
        'songid': ('int', 'track'),
        'playlist': ('int', 'track'),
        'repeat': ('int', 'repeat'),
        'single': ('int', 'repeat'),
        'random': ('int', 'shuffle'),
        'state': ('_parse_status', 'status'),
    }

    def __init__(self, proxy, playername):
        super(MpdPlayer, self).__init__(proxy, playername)
        self._inited = False
        self._metadata = None
        self._songid = -1
        self._playlist = -1
        self._repeat = None
        self._single = None
        self._random = None
        self._state = None
        self._elapsed = Timer(100)
        self._send_cmd(Cmds.STATUS, sync=True)
        self._inited = True

    def _send_cmd(self, cmd, *args, **kwargs):
        """ Send a cmd. Can use sync=[True|False] to send in a blocking or
        non-blocking way. Default is non-blocking
        """
        sync = False if 'sync' not in kwargs else kwargs['sync']
        if cmd not in self.CMD_HANDLERS:
            raise RuntimeError('Unknown command: %s', cmd)
        handler = self.CMD_HANDLERS[cmd]
        if handler is not None:
            handler = getattr(self, handler)
        else:
            handler = self._handle_nothing
        if sync:
            self.proxy.send_command_sync(cmd, handler, *args)
        else:
            self.proxy.send_command(cmd, handler, *args)

    def _handle_status(self, status):
        logging.debug('status\n%s', status)
        changes = set()
        for prop, handler in self.STATUS_CHANGE_MAP.items():
            if not prop in status:
                value = None
            else:
                funcname = handler[0]
                func = getattr(__builtin__, funcname) \
                    if not funcname.startswith('_') \
                    else getattr(self, funcname)
                value = func(status[prop])
            if value != getattr(self, '_' + prop):
                logging.debug('prop %s changed to %s', prop, value)
                setattr(self, '_' + prop, value)
                changes.add(handler[1])

        if 'track' in changes:
            if self._songid is None:
                self._metadata = Metadata()
            else:
                self._send_cmd(Cmds.CURRENTSONG, sync=True)

        if 'status' in changes:
            if self._state == STATUS_PAUSED:
                self._elapsed.pause()
            elif self._state == STATUS_PLAYING:
                self._elapsed.play()
            else:
                self._elapsed.stop()
        if self._state == STATUS_STOPPED:
            elapsed = 0
        else:
            elapsed = float(status['elapsed']) * 1000
        if self._elapsed.set_time(elapsed):
            changes.add('position')
        if not self._inited:
            # Initializing, do not emit the change signals
            changes = set()
        for change in changes:
            getattr(self, change + '_changed')()

    def _handle_currentsong(self, metadata):
        logging.debug('currentsong: %s', metadata)
        args = {}
        for key in ('title', 'artist', 'album'):
            if key in metadata:
                args[key] = metadata[key]
        if 'time' in metadata:
            args['length'] = int(metadata['time']) * 1000
        if 'track' in metadata:
            args['tracknum'] = int(metadata['track'].split('/')[0])
        self._metadata = Metadata(**args)

    def _parse_status(self, value):
        status_map = {
            'play': STATUS_PLAYING,
            'pause': STATUS_PAUSED,
            'stop': STATUS_STOPPED,
        }
        if value not in status_map:
            raise RuntimeError('Unknown status ' + value)
        return status_map[value]

    def _handle_replay_gain_status(self, status):
        pass

    def _handle_nothing(self, *args):
        pass

    def handle_changes(self, changes):
        cmds = set()
        for change in changes:
            if change in self.CHANGE_CMDS:
                for cmd in self.CHANGE_CMDS[change]:
                    cmds.add(cmd)
        logging.debug('changes: %s', changes)
        logging.debug('cmds: %s', cmds)
        for cmd in cmds:
            self._send_cmd(cmd)

    def get_status(self):
        return self._state

    def get_metadata(self):
        return self._metadata

    def get_position(self):
        return self._elapsed.time

    def get_caps(self):
        return set([CAPS_PLAY, CAPS_PAUSE, CAPS_NEXT, CAPS_PREV, CAPS_SEEK])

    def get_repeat(self):
        if not self._repeat:
            return REPEAT_NONE
        if not self._single:
            return REPEAT_ALL
        return REPEAT_TRACK

    def set_repeat(self, mode):
        repeat_mode_map = {
            REPEAT_NONE: (0, 0),
            REPEAT_TRACK: (1, 1),
            REPEAT_ALL: (1, 0),
        }
        if mode not in repeat_mode_map:
            raise ValueError('Unknown repeat mode: %s', mode)
        self._repeat = repeat_mode_map[mode][0]
        self._single = repeat_mode_map[mode][1]
        self._send_cmd(Cmds.REPEAT, self._repeat)
        self._send_cmd(Cmds.SINGLE, self._single)

    def get_shuffle(self):
        return bool(self._random)

    def set_shuffle(self, shuffle):
        self._random = 1 if shuffle else 0
        self._send_cmd(Cmds.RANDOM, self._random)

    def play(self):
        if self._state == STATUS_PAUSED:
            self._send_cmd(Cmds.PAUSE, 0)
        elif self._state == STATUS_STOPPED:
            self._send_cmd(Cmds.PLAY)

    def pause(self):
        self._send_cmd(Cmds.PAUSE, 1)

    def stop(self):
        self._send_cmd(Cmds.STOP)

    def prev(self):
        self._send_cmd(Cmds.PREVIOUS)

    def next(self):
        self._send_cmd(Cmds.NEXT)

    def set_position(self, pos):
        self._send_cmd(Cmds.SEEK, self._songid, int(pos / 1000))

    def debug_info(self):
        ret = dbus.Dictionary(signature='sv')
        ret.update({
            'state': self._state,
            'metadata': self._metadata.to_mpris1(),
            'repeat': self._repeat,
            'single': self._single,
            'position': self.get_position()
        })
        return ret

if __name__ == '__main__':
    proxy = MpdProxy()
    proxy.run()
