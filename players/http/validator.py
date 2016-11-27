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

from error import BadRequestError

__all__ = (
    'baseparam',
    'param_int',
    'param_str',
    'param_enum',
    'param_set',
    'validate_params',
    )

class baseparam(object):
    def __init__(self, optional=False):
        self.optional = optional

    def validate(self, value):
        raise NotImplementedError()

class param_int(baseparam):
    def __init__(self, max=None, min=None, optional=False):
        baseparam.__init__(self, optional)
        self._max = max
        self._min = min

    def validate(self, value):
        try:
            v = int(value)
            if (self._max is None or self._max <= v) and \
                    (self._min is None or self._min >= v):
                return True, v
            else:
                return False, v
        except:
            return False, value

class param_str(baseparam):
    def __init__(self, nonempty=False, optional=False):
        baseparam.__init__(self, optional)
        self._nonempty = nonempty
        
    def validate(self, value):
        return True if not self._nonempty else isinstance(value, basestring), value

class param_enum(baseparam):
    def __init__(self, valid_values, optional=False):
        baseparam.__init__(self, optional)
        self._valid_values = valid_values
        
    def validate(self, value):
        if not value in self._valid_values:
            return False, ''
        try:
            v = self._valid_values[value]
            return True, v
        except:
            return True, value

class param_set(baseparam):
    def __init__(self, valid_values, optional=False):
        baseparam.__init__(self, optional)
        self._valid_values = valid_values
        
    def validate(self, value):
        ret = set()
        for k in value.split(','):
            k = k.strip()
            
            if not k in self._valid_values:
                return False, ret
            try:
                v = self._valid_values[k]
                ret.add(v)
            except:
                ret.add(k)
        return True, ret

def validate_params(param_def):
    def dec(func):
        def dec_func(handler, params, *args, **kargs):
            valid_params = {}
            for k, v in params.items():
                if k in param_def:
                    valid, value = param_def[k].validate(v)
                    if not valid:
                        raise BadRequestError('query "%s=%s" is invalid' % (k, v))
                    v = value
                valid_params[k] = v
            for k, v in param_def.items():
                if not v.optional and not k in params:
                    raise BadRequestError('missing "%s" in query' % k)
            return func(handler, valid_params, *args, **kargs)
        return dec_func
    return dec
