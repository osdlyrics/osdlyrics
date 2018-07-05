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
from future import standard_library
standard_library.install_aliases()
import http.client


class InvalidRequestException(Exception):

    def __init__(self, message):
        self._message = message

    def __str__(self):
        return repr(self._message)


class HttpError(Exception):
    """
    Error response
    """

    def __init__(self, code, message=''):
        self.code = code
        self.message = message


class NotFoundError(HttpError):
    """
    HTTP Error 404
    """

    def __init__(self, message=''):
        HttpError.__init__(self, http.client.NOT_FOUND, message)


class BadRequestError(HttpError):
    """
    HTTP Error 400
    """

    def __init__(self, message=''):
        HttpError.__init__(self, http.client.BAD_REQUEST, message)


class PlayerNotFoundError(Exception):
    pass
