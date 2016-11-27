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

import consts

class Config(object):
    """ Helper class to retrive configs from OSD Lyrics through DBus

    It provides a set of get_<type> functions to get config values with default
    values. If the value with given key exists, return the value. Otherwise the value
    to the key is set to the default value, and returns it. If no default value
    specified and the key does not exist, raise an exception.

    Values can be monitored by connect_change function.
    """
    
    def __init__(self, conn, follow_name_owner_changes=True):
        """
        Arguments:
        - `conn`: DBus connection
        """
        self._conn = conn
        self._proxy = conn.get_object(consts.CONFIG_BUS_NAME,
                                      consts.CONFIG_OBJECT_PATH,
                                      follow_name_owner_changes=follow_name_owner_changes)
        self._proxy = dbus.Interface(self._proxy,
                                     consts.CONFIG_INTERFACE)
        self._signals = {}
        self._proxy.connect_to_signal('ValueChanged',
                                      self._value_changed_cb)

    def get_bool(self, key, default=None):
        try:
            return self._proxy.GetBool(key)
        except Exception as e:
            if default is not None:
                try:
                    self._proxy.SetBool(key, default)
                except:
                    pass
                return default
            raise e

    def set_bool(self, key, value):
        self._proxy.SetBool(key, value)

    def get_int(self, key, default=None):
        try:
            return self._proxy.GetInt(key)
        except Exception as e:
            if default is not None:
                try:
                    self._proxy.SetInt(key, default)
                except:
                    pass
                return default
            raise e

    def set_int(self, key, value):
        self._proxy.SetInt(key, value)

    def get_double(self, key, default=None):
        try:
            return self._proxy.GetDouble(key)
        except Exception as e:
            if default is not None:
                try:
                    self._proxy.SetBool(key, default)
                except:
                    pass
                return default
            raise e

    def set_double(self, key, value):
        self._proxy.SetDouble(key, value)

    def get_string(self, key, default=None):
        try:
            return self._proxy.GetString(key)
        except Exception as e:
            if default is not None:
                try:
                    self._proxy.SetString(key, default)
                except:
                    pass
                return default
            raise e

    def set_string(self, key, value):
        self._proxy.SetString(key, value)

    def get_string_list(self, key, default=None):
        try:
            return self._proxy.GetStringList(key)
        except Exception as e:
            if default is not None:
                try:
                    self._proxy.SetStringList(key, default)
                except:
                    pass
                return default
            raise e

    def set_string_list(self, key, value):
        self._proxy.SetStringList(key, value)

    def connect_change(self, key, func):
        """
        Calls func once a config value with specified key is changed

        The callback function `func` receives one argument, the key that
        has changed
        """
        if not callable(func):
            return
        self._signals.setdefault(key, []).append(func)

    def disconnect_change(self, key=None, func=None):
        if key is None:
            self._signals = {}
            return
        if key in self._signals:
            if func is None:
                del self._signals[key]
            else:
                self._signals[key].remove(func)

    def _value_changed_cb(self, name_list):
        for name in name_list:
            if name in self._signals:
               for handler in self._signals[name]:
                   handler(name)

def test():
    def value_changed(name):
        typename = name.split('/')[1]
        logging.debug('%s has been changed to %s' % (name,
                                                     getattr(config, 'get_' + typename)(name)))
    
    import glib
    from dbus.mainloop.glib import DBusGMainLoop
    loop = glib.MainLoop()
    dbus_mainloop = DBusGMainLoop()
    conn = dbus.SessionBus(mainloop=dbus_mainloop)
    config = Config(conn)
    testcase = { 'bool': False,
                 'int': 123,
                 'double': 123.54,
                 'string': 'Foobar',
                 'string_list': ['Foo', 'bar'],
                 }
    for k in testcase.keys():
        config.connect_change('test/' + k, value_changed)
    for k, v in testcase.items():
        getattr(config, 'set_' + k)('test/' + k, v)
    loop.run()

if __name__ == '__main__':
    test()
