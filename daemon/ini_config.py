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

import ConfigParser

import dbus
import dbus.service
import glib

from osdlyrics.app import App
from osdlyrics.consts import CONFIG_BUS_NAME, CONFIG_OBJECT_PATH
import osdlyrics.errors
import osdlyrics.utils


class MalformedKeyError(osdlyrics.errors.BaseError):
    def __init__(self, *args):
        super(MalformedKeyError, self).__init__(*args)

class ValueNotExistError(osdlyrics.errors.BaseError):
    def __init__(self, key=''):
        super(ValueNotExistError, self).__init__(
            'Value of key %s does not exist' % key
            )

class IniConfig(dbus.service.Object):
    """ Implement org.osdlyrics.Config
    """

    def __init__(self,
                 conn,
                 filename=osdlyrics.utils.get_config_path('osdlyrics.conf')):
        super(IniConfig, self).__init__(conn=conn,
                                        object_path=CONFIG_OBJECT_PATH)
        self._conn = conn
        self._confparser = ConfigParser.RawConfigParser()
        osdlyrics.utils.ensure_path(filename)
        self._confparser.read(filename)
        self._filename = filename
        self._save_timer = None
        self._signal_timer = None
        self._changed_signals = {}

    def _split_key(self, key, add_section=True):
        parts = key.split('/')
        if len(parts) != 2:
            raise MalformedKeyError(
                '%s is an invalid key. Keys must be in the form '
                'of "Section/Name"' % key
                )
        if len(parts[0]) == 0 or len(parts[1]) == 0:
            raise MalformedKeyError(
                'Malformed key "%s". Section or name must not be empty' % key)
        if add_section and not self._confparser.has_section(parts[0]):
            self._confparser.add_section(parts[0])
        return parts[0], parts[1]

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='s',
                         out_signature='b')
    def GetBool(self, key):
        section, name = self._split_key(key)
        try:
            return self._confparser.getboolean(section, name)
        except:
            raise ValueNotExistError(key)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='s',
                         out_signature='i')
    def GetInt(self, key):
        section, name = self._split_key(key)
        try:
            return self._confparser.getint(section, name)
        except:
            raise ValueNotExistError(key)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='s',
                         out_signature='d')
    def GetDouble(self, key):
        section, name = self._split_key(key)
        try:
            return self._confparser.getfloat(section, name)
        except:
            raise ValueNotExistError(key)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='s',
                         out_signature='s')
    def GetString(self, key):
        section, name = self._split_key(key)
        try:
            return self._confparser.get(section, name)
        except:
            raise ValueNotExistError(key)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='s',
                         out_signature='as')
    def GetStringList(self, key):
        value = self.GetString(key)
        try:
            return split(value)
        except:
            raise ValueNotExistError(key)

    def _set_value(self, key, value, overwrite=True):
        section, name = self._split_key(key, True)
        if overwrite or not self._confparser.has_option(section, name):
            self._confparser.set(section, name, str(value))
            self._changed_signals[key] = True
            self._schedule_save()
            self._schedule_signal()

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='sb',
                         out_signature='')
    def SetBool(self, key, value):
        self._set_value(key, 'true' if value else 'false')

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='si',
                         out_signature='')
    def SetInt(self, key, value):
        self._set_value(key, value)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='sd',
                         out_signature='')
    def SetDouble(self, key, value):
        self._set_value(key, value)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='ss',
                         out_signature='')
    def SetString(self, key, value):
        self._set_value(key, value)

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='sas',
                         out_signature='')
    def SetStringList(self, key, value):
        self._set_value(key, join(value))

    @dbus.service.method(dbus_interface=CONFIG_BUS_NAME,
                         in_signature='a{sv}',
                         out_signature='')
    def SetDefaultValues(self, values):
        for k, v in values.items():
            if isinstance(v, list):
                v = join(v)
            self._set_value(k, v, False)

    def _schedule_save(self, filename=None):
        if self._save_timer is None:
            self._save_timer = glib.timeout_add(1000,
                                                lambda: self.save(filename))

    def save(self, filename=None):
        if filename is None:
            filename = self._filename
        self._confparser.write(open(filename, 'w'))
        if self._save_timer is not None:
            glib.source_remove(self._save_timer)
            self._save_timer = None

    def _schedule_signal(self):
        if self._signal_timer is None:
            self._signal_timer = glib.timeout_add(500,
                                                  lambda: self.emit_change())

    def emit_change(self):
        if self._signal_timer is not None:
            glib.source_remove(self._signal_timer)
            self._signal_timer = None
        changed = []
        for key in self._changed_signals.keys():
            changed.append(key)
        self.ValueChanged(changed)
        self._changed_signals = {}

    @dbus.service.signal(dbus_interface=CONFIG_BUS_NAME,
                         signature='as')
    def ValueChanged(self, changed):
        pass

def split(value, sep=';'):
    r"""
    >>> split('')
    []
    >>> split(' ')
    [' ']
    >>> split('single')
    ['single']
    >>> split('one;two')
    ['one', 'two']
    >>> split('one;')
    ['one']
    >>> split(';one;two;')
    ['', 'one', 'two']
    >>> split(r'one\;two;three\\;four')
    ['one;two', 'three\\', 'four']
    >>> split(r'\one\\\;two;\\three\\\\;four;')
    ['\\one\\;two', '\\three\\\\', 'four']
    >>> split('; ')
    ['', ' ']
    """
    start = 0
    ret = []
    item = []
    curr = 0
    while curr <= len(value):
        if curr == len(value) or value[curr] == sep:
            if start < curr:
                item.append(value[start:curr])
            if curr != len(value) or len(item) > 0:
                ret.append(''.join(item))
            item = []
            start = curr + 1
        elif value[curr] == '\\' and curr < len(value) - 1:
            tag = value[curr + 1]
            if tag == '\\' or tag == sep:
                item.append(value[start:curr])
                start = curr + 1
                curr = start
        curr = curr + 1
    return ret

def join(values, sep=';'):
    r"""
    >>> join([])
    ''
    >>> join([''])
    ';'
    >>> join(['one'])
    'one;'
    >>> join(['one', 'two'])
    'one;two;'
    >>> join(['one;', 'two'])
    'one\\;;two;'
    >>> print join([r'on\e', 't;wo'])
    on\\e;t\;wo;
    """
    if len(values) == 0:
        return ''
    result = []
    for item in values:
        result.append(item.replace('\\', '\\\\').replace(sep, '\\;'))
    return sep.join(result) + sep

def test():
    import doctest
    doctest.testmod()

def run():
    app = App('Config')
    if len(sys.argv) > 1:
        ini_conf = IniConfig(app.connection, sys.argv[1])
    else:
        ini_conf = IniConfig(app.connection)
    app.run()

if __name__ == '__main__':
    import sys
    if '--test' in sys.argv:
        test()
    else:
        run()
