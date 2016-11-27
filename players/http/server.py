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

import BaseHTTPServer
import json
import logging
import urlparse

from osdlyrics.metadata import Metadata
from osdlyrics.player_proxy import (CAPS_NEXT, CAPS_PAUSE, CAPS_PLAY,
    CAPS_PREV, CAPS_SEEK, STATUS_PAUSED, STATUS_PLAYING, STATUS_STOPPED)

from error import BadRequestError, HttpError, NotFoundError
from validator import (param_enum, param_int, param_set, param_str,
                       validate_params)

PARAM_STATUS = param_enum({'playing': STATUS_PLAYING,
                           'paused': STATUS_PAUSED,
                           'stopped': STATUS_STOPPED})
PARAM_CAPS = param_set({'play': CAPS_PLAY,
                        'pause': CAPS_PAUSE,
                        'next': CAPS_NEXT,
                        'prev': CAPS_PREV,
                        'seek': CAPS_SEEK})

def parse_query(query):
    """ Parse query strings in GET or POST to a dict

    The return value is a dictionary with query keys as query names and
    values as the query values. If a query name does not have a value, the
    value to the key is True. If more than one value assigned to the query name,
    any one may be assigned to the key.
    
    Arguments:
    - `query`: A string like 'query1=value&query2=value'
    """
    result = urlparse.parse_qs(query)
    ret = {}
    for k, v in result.items():
        if len(v) == 0:
            ret[k] = True
        else:
            ret[k] = v[0]
    return ret

class RequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    """ Handles HTTP request
    """
    
    server_version = 'OsdLyricsHttp/1.0'

    def _send_content(self, content):
        self.send_response(200)
        self.end_headers()
        self.wfile.write(content)

    def _send_error(self, error):
        self.send_error(error.code, error.message)

    def _processquery(self, params):
        url = urlparse.urlparse(self.path)
        cmd = url.path[1:]
        if hasattr(self, 'do_' + cmd):
            try:
                content = getattr(self, 'do_' + cmd)(params)
                self._send_content(content)
            except HttpError as e:
                self._send_error(e)
        else:
            self._send_error(NotFoundError('Invalid request: %s' % cmd))

    def do_GET(self):
        url = urlparse.urlparse(self.path)
        params = parse_query(url.query)
        self._processquery(params)

    @validate_params({'name': param_str(),
                      'caps': PARAM_CAPS,
                      })
    def do_connect(self, params):
        logging.debug('caps: %s', params['caps'])
        return json.dumps({'id': self.server.player_proxy.add_player(params['name'],
                                                                     params['caps']),
                           })

    @validate_params({'id': param_str(),
                      'timestamp': param_int(),
                      })
    def do_query(self, params):
        cmds, timestamp = self.get_player(params['id']).query(params['timestamp'])
        return json.dumps({'cmds': cmds, 'timestamp': timestamp})

    @validate_params({'id': param_str(),
                      'status': PARAM_STATUS,
                      'title': param_str(),
                      'artist': param_str(optional=True),
                      'album': param_str(optional=True),
                      'arturl': param_str(optional=True),
                      'tracknum': param_int(optional=True),
                      'length': param_int()
                      })
    def do_track_changed(self, params):
        player = self.get_player(params['id'])
        status = params['status']
        del params['status']
        del params['id']
        metadata = Metadata(**params)
        player.do_update_track(metadata)
        player.do_update_status(status)
        return ''

    @validate_params({'id': param_str(),
                      'status': PARAM_STATUS,
                      })
    def do_status_changed(self, params):
        player = self.get_player(params['id'])
        player.do_update_status(params['status'])

    @validate_params({'id': param_str(),
                      'pos': param_int()})
    def do_position_changed(self, params):
        player = self.get_player(params['id'])
        player.do_update_position(params['pos'])

    @validate_params({'id': param_str()})
    def do_disconnect(self, params):
        player = self.get_player(params['id'])
        player.disconnect()

    def get_player(self, name):
        try:
            return self.server.player_proxy.get_player(name)
        except:
            raise BadRequestError('Invalid player id: %s' % name)
        

class HttpServer(BaseHTTPServer.HTTPServer):
    """
    Lyrics Http server
    """
    
    def __init__(self, server_address, player_proxy):
        """
        
        Arguments:
        - `server_address`:
        """
        BaseHTTPServer.HTTPServer.__init__(self,
                                           server_address,
                                           RequestHandler)
        self._player_conter = 1
        self._connected_players = {}
        self._player_proxy = player_proxy

    @property
    def player_proxy(self):
        return self._player_proxy
        
