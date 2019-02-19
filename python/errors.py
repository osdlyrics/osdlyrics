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
# along with OSD Lyrics.  If not, see <https://www.gnu.org/licenses/>.
#/

import dbus.exceptions

class BaseError(dbus.exceptions.DBusException):
    """ Base class for raising an exception through D-Bus
    """

    def __init__(self, *args, **kwargs):
        """

        Arguments:
        - `name`: The name of exception to send through D-Bus. If not set,
          'org.osdlyrics.Error.' + the name of the class (without 'Error'
          suffix, if any) will be returned.
        - `*args`:
        - `*kwargs`:
        """
        dbus_error_name = kwargs.pop('name', None)
        if dbus_error_name is None:
            error_name = self.__class__.__name__
            if error_name.endswith('Error'):
                error_name = error_name[:-len('Error')]
            dbus_error_name = 'org.osdlyrics.Error.' + error_name
        kwargs['name'] = dbus_error_name
        dbus.exceptions.DBusException.__init__(self, *args, **kwargs)

class PatternException(Exception):
    pass
