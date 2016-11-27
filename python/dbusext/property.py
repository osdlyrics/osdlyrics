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

import dbus.exceptions


class AccessDeniedError(dbus.exceptions.DBusException):
    def __init__(self, *args):
        dbus.exceptions.DBusException.__init__(self, dbus_error_name='org.osdlyrics.Error.AccessDenied', *args)


class Property(object):
    """ DBus property class
    """

    def __init__(self, dbus_interface, type_signature,
                 emit_change=True, readable=True, writeable=True,
                 name=None, fget=None, fset=None, dbus_set=None):
        """

        Arguments:
        - `type_signature`: (string) Type signature of this property. This parameter
          is used for introspection only
        - `dbus_interface`: (string) The DBus interface of this property
        - `emit_change`: (boolean or string) Whether to emit change with
                        `PropertiesChanged` D-Bus signal when the property is set.
                        Possible values are boolean value True or False, or a string
                        'invalidates'.
        - `readable`: Whether the property is able to visit with `Get` D-Bus method.
        - `writeable`: Whether the property is able to write with `Set` D-Bus method.
                       A property is writeable only when `writeable` is set to True
                       and a setter function is set.
        """
        self._type_signature = type_signature
        # we use two underscores because dbus-python uses _dbus_interface to determine
        # whether an object is a dbus method
        self.__dbus_interface = dbus_interface
        self._fset = fset
        self._fget = fget
        self.__name__ = name
        self._dbusset = dbus_set
        if not emit_change in [True, False, 'invalidates']:
            raise ValueError('Value of emit_change must be one of True, False, or \'invalidates\'')
        self._emit_change = emit_change
        self._readable = readable
        self._writeable = writeable

    @property
    def interface(self):
        """ Return the dbus interface of this property
        """
        return self.__dbus_interface

    @property
    def readable(self):
        return self._readable

    @property
    def writeable(self):
        return self._writeable

    @property
    def emit_change(self):
        return str(self._emit_change).lower()

    @property
    def type_signature(self):
        return self._type_signature

    def __get__(self, obj, objtype=None):
        if obj is None:
            return self
        if self._fget is None:
            raise AttributeError("unreadable attribute")
        return wrap_dbus_type(self._type_signature, self._fget(obj))

    def __set__(self, obj, value):
        if self._fset is None:
            raise AttributeError("can't set attribute")
        self._set_value(obj, value, self._fset)

    def dbus_set(self, obj, value):
        """
        Set a property through D-Bus.

        This is intended to called by dbusext.service.Object to implement D-Bus
        property features. Don't call it directly.

        If dbus setter is set by `dbus_setter` decorator, the dbus setter will be
        used the handle this request. Otherwise, if `writeable` is set to `True`,
        the general setter set by `setter` decorator will be used.
        """
        if callable(self._dbusset):
            self._set_value(obj, value, self._dbusset)
        elif self.writeable and callable(self._fset):
            self._set_value(obj, value, self._fset)
        else:
            raise AccessDeniedError('Property %s is not writeable' % self.__name__)

    def _set_value(self, obj, value, setter):
        changed = setter(obj, value)
        if not self._emit_change:
            return
        if changed is None or changed:
            changed = True
        else:
            changed = False
        if changed and getattr(self, '__name__', None) and getattr(obj, '_property_set', None):
            obj._property_set(self.__name__, self._emit_change == True)

    def setter(self, fset):
        """
        Sets the setter function of the property. Return the property object itself.

        The setter function should return a boolean value to tell if the value
        is changed. Returning None is considered as True.

        When ``emit_change`` is True in constructor, the property will emit changes
        when value is set. If the owner object has a function named
        ``_property_set``, and the ``__name__`` attribute of the property object is
        set by the class, the function will be invoked to notify the property has
        been set when the setter is called, with the name of property as the first
        argument, and an boolean as the second argument to tell whether the value
        is changed.

        This is usually used as an decorator::

            class A(osdlyrics.dbus.Object):

              @osdlyrics.dbus.property(type='s', dbus_interface='example.property')
              def x(self):
                  return self._x

              @x.setter
              def x(self, value):
                  if self._x != value:
                      self._x = value
                      return True
                  else:
                      return False

              @x.dbus_setter
              def x(self, value):
                  if self._do_something(value):
                      self._x = value
                      return True
                  return False

        By default, the setter is used as both python property setter and dbus
        property setter. If you want a different set-handler to handle dbus property
        set request, use `dbus_setter`.
        """
        self._fset = fset
        return self

    def dbus_setter(self, fset):
        """
        A decorator to create a D-Bus property set handler.

        The new value set through D-Bus interface is passed to this setter. If dbus
        setter not set, the general setter set by `Property.setter` decorator is
        used as D-Bus property set handler.

        Arguments:
        - `fset`: The setter handler. Takes a parameter as the new value, and return
                  a boolean value to indicate whether the property is changed. If
                  None is returned, it is treated as True. See `setter` for example.
        """
        self._dbusset = fset
        return self


DBUS_TYPE_MAP = {
    'y': dbus.Byte,
    'b': dbus.Boolean,
    'n': dbus.Int16,
    'q': dbus.UInt16,
    'i': dbus.Int32,
    'u': dbus.UInt32,
    'x': dbus.Int64,
    't': dbus.UInt64,
    'd': dbus.Double,
    's': dbus.String,
    'o': dbus.ObjectPath,
    'g': dbus.Signature,
    }


def wrap_dbus_type(signature, value):
    if signature in DBUS_TYPE_MAP:
        dbustype = DBUS_TYPE_MAP[signature]
        if isinstance(value, dbustype):
            return value
        else:
            return dbustype(value)
    elif signature.startswith('a{'):
        if isinstance(value, dbus.Dictionary):
            return value
        else:
            return dbus.Dictionary(value, signature=signature)
    elif signature.startswith('a'):
        if isinstance(value, dbus.Array):
            return value
        else:
            return dbus.Array(value, signature=signature)
    elif signature.startswith('('):
        if isinstance(value, dbus.Struct):
            return value
        else:
            return dbus.Struct(value, signature=signature)
