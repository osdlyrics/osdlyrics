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

import os
import os.path
import stat
import urllib
import pycurl
import StringIO
import sys
import urlparse

__all__ = (
    'cmd_exists',
    'ensure_utf8',
    'ensure_unicode',
    'ensure_path',
    'get_config_path',
    'http_download',
    'path2uri',
    )

pycurl.global_init(pycurl.GLOBAL_DEFAULT)

# make sure the default encoding is utf-8
if sys.getdefaultencoding() != 'utf-8':
    reload(sys)
    sys.setdefaultencoding('utf-8')

class ProxySettings(object):
    """
    """

    def __init__(self, protocol, host='', port=0, username=None, password=None):
        """
        """
        self.protocol = protocol
        self.host = host
        self.port = port
        self.username = username
        self.password = password

def get_config_path(filename='', expanduser=True):
    """
    Gets the path to save config files

    Arguments:
    - `filename`: (optional string) The filename of config file.
    - `expanduser`: (optional bool) If the leading "~" should be expanded as user's
      home directory

    >>> get_config_path(expanduser=False)
    '~/.config/osdlyrics/'
    >>> get_config_path('osdlyrics.conf', False)
    '~/.config/osdlyrics/osdlyrics.conf'
    """
    path = os.path.join('~/.config/osdlyrics/', filename)
    if expanduser:
        path = os.path.expanduser(path)
    return path

def path2uri(path):
    r"""
    Converts a path to URI with file sheme.

    If a path does not start with a slash (/), it is considered to be an invalid
    path and returned directly.

    >>> path2uri('/path/to/file')
    'file:///path/to/file'
    >>> path2uri('file:///path/to/file')
    'file:///path/to/file'
    >>> path2uri(u'/path/to/file')
    'file:///path/to/file'
    >>> path2uri('invalid/path')
    'invalid/path'
    >>> path2uri('/\xe8\xb7\xaf\xe5\xbe\x84/\xe6\x96\x87\xe4\xbb\xb6')
    'file:///%E8%B7%AF%E5%BE%84/%E6%96%87%E4%BB%B6'
    """
    if path.startswith('~'):
        path = os.path.expanduser(path)
    if not path.startswith('/'):
        return path
    if isinstance(path, unicode):
        path = path.encode('utf8')
    return 'file://' + urllib.pathname2url(path)

def ensure_unicode(value):
    r"""
    If value is a string, decode with utf-8. Otherwise return it directly.
    """
    if isinstance(value, str):
        return value.decode('utf8')
    return value

def ensure_utf8(value):
    r"""
    If value is a unicode, encode with utf-8. Otherwise return it directly.
    """
    if isinstance(value, unicode):
        return value.encode('utf8')
    return value

def get_proxy_settings(config=None, conn=None):
    r"""
    Return proxy settings as a ProxySettings object

    The caller must specify either config or conn.

    Arguments:
     - `config`: A osdlyrics.config.Config object, this object is used to retrive
                 proxy settings. If it is not set, the caller MUST set conn to a
                 valid D-Bus connection to create a Config object
     - `conn`: A D-Bus connection object, this is used when `config` is not
               specified.
    """
    if config is None and conn is None:
        raise ValueError('Either config or conn must be specified')
    if config is None:
        config = config.Config(conn)
    proxy_type = config.get_string('Download/proxy')
    if proxy_type.lower() == 'no':
        return ProxySettings(protocol='no')
    if proxy_type.lower() == 'manual':
        protocol = config.get_string('Download/proxy-type')
        host = config.get_string('Download/proxy-host')
        port = config.get_int('Download/proxy-port')
        username = config.get_string('Download/proxy-username')
        passwd = config.get_string('Download/proxy-passwd')
        return ProxySettings(protocol=protocol, host=host, port=port,
                            username=username, password=passwd)
    if proxy_type.lower() == 'system':
        return detect_system_proxy()

def detect_system_proxy():
    r"""
    Detects and return system proxy settings.

    Support following proxy settings:
    - Environment variables
    - GNOME 2 (TODO)
    - GNOME 3
    - KDE
    """
    desktop = detect_desktop_shell()
    if desktop == 'gnome' or desktop == 'unity':
        proxy = get_gsettings_proxy()
        if proxy is not None:
            return proxy
    elif desktop == 'kde':
        proxy = get_kde_proxy()
        if proxy is not None:
            return proxy
    return get_envar_proxy()

def get_envar_proxy():
    r"""
    Return proxy settings from environment variable `http_proxy`
    """
    envars = ['http_proxy', 'HTTP_PROXY']
    proxies = [os.environ.get(v) for v in envars]
    for proxy in proxies:
        if proxy is not None:
            if proxy.find('://') < 0:
                proxy = 'http://' + proxy
            parts = urlparse.urlparse(proxy)
            if not parts.scheme in ['http', 'socks4', 'socks5', '']:
                continue
            return ProxySettings(protocol=parts.scheme if parts.scheme != '' else 'http',
                                 host=parts.hostname,
                                 port=parts.port if parts.port is not None else 8080,
                                 username=parts.username,
                                 password=parts.password)
    return ProxySettings(protocol='no')

def detect_desktop_shell():
    r"""
    Detect the currently running destop shell.

    Returns: 'gnome', 'unity', 'kde', or 'unknown'
    """
    envar = os.environ.get('DESKTOP_SESSION')
    if envar.startswith('gnome'):
        return 'gnome'
    if envar.startswith('kde'):
        return 'kde'
    if envar.startswith('ubuntu') or envar.startswith('unity'):
        return 'unity'
    return 'unknown'

def get_gsettings_proxy():
    r"""
    Return proxy settings from gsetting, this is used in GNOME 3
    """
    try:
        from gi.repository import Gio
    except:
        return None
    if not hasattr(Gio, 'Settings'):
        return None
    if 'org.gnome.system.proxy' not in Gio.Settings.list_schemas():
        return None
    settings = Gio.Settings('org.gnome.system.proxy')
    if settings.get_string('mode') != 'manual':
        return ProxySettings(protocol='no')
    protocol_map = { 'http': 'http', 'socks5': 'socks' }
    for protocol, key in protocol_map.items():
        settings = Gio.Settings('org.gnome.system.proxy.' + key)
        host = settings.get_string('host').strip()
        port = settings.get_int('port')
        if host == '' or port <= 0:
            continue
        username = ''
        password = ''
        if key == 'http' and settings.get_boolean('use-authentication'):
            username = settings.get_string('authentication-user')
            password = settings.get_string('authentication-password')
        return ProxySettings(protocol=protocol,
                             host=host,
                             port=port,
                             username=username,
                             password=password)
    return ProxySettings(protocol='no')

def get_kde_proxy():
    r"""
    Detect KDE4 proxy settings
    """
    try:
        import PyKDE4.kdecore as kdecore
    except:
        return None
    config = kdecore.KConfig('kioslaverc', kdecore.KConfig.NoGlobals)
    if not config.hasGroup('Proxy Settings'):
        return None
    group = config.group('Proxy Settings')
    proxy_type, _ = group.readEntry('ProxyType', 0).toInt()
    if proxy_type in [0, 2, 3]:
        # no proxy, PAC or auto detect in KDE settings. We don't support
        # PAC proxy, so treat them as no proxy
        return ProxySettings('no')
    elif proxy_type in [1, 4]:
        for key in ['httpProxy', 'socksProxy']:
            value = str(group.readEntry(key))
            if value.strip() != '':
                # KDE 4.8 uses whitespace to seperate port and hostname
                value = value.replace(' ', ':')
                if value.find('://') < 0:
                    value = 'http://' + value
                parts = urlparse.urlparse(value)
                host = parts.hostname
                try:
                    port = parts.port
                except:
                    port = None
                if host is not None and host.strip() != '' and \
                        port is not None and port > 0 and port < 65536:
                    protocolmap = {'httpProxy': 'http',
                                   'socksProxy': 'socks5'}
                    return ProxySettings(protocolmap[key],
                                         host=host,
                                         port=port)
    return ProxySettings('no')

def http_download(url, port=0, method='GET', params={}, headers={}, timeout=15, proxy=None):
    r"""
    Helper function to download files from website

    This function will apply proxy settings and deal redirections automatically.
    To apply proxy settings, pass an ProxySettings object as the `proxy` parameter.

    If `'User-Agent'` is not set in `headers`, it will be set to `'OSD Lyrics'`.

    Arguments:
     - `url`: The url of the content
     - `port`: (optional) The port.
     - `method`: (optional) The HTTP method to download contents. Available values
                 are `'POST'` or `'GET'`. The default value is `'GET'`.
     - `params`: (optional) The parameters of the request. It is a dict. If `method`
                 is `'GET'`, `params` will be encoded and append to the url as the
                 param part. If `method` is `'POST'`, `params` will be added to
                 request headers as post data.
     - `headers`: (optional) A dict of HTTP headers.
     - `proxy`: (optional) A ProxySettings object to sepcify the proxy to use.

    >>> code, content = http_download('http://www.python.org/')
    >>> code
    200
    >>> content.find('Python') >= 0
    True
    """
    c = pycurl.Curl()
    buf = StringIO.StringIO()
    c.setopt(pycurl.NOSIGNAL, 1)
    c.setopt(pycurl.DNS_USE_GLOBAL_CACHE, 0)
    c.setopt(pycurl.FOLLOWLOCATION, 1)
    c.setopt(pycurl.MAXREDIRS, 5)
    c.setopt(pycurl.WRITEFUNCTION, buf.write)
    if method == 'GET' and len(params) > 0:
        params = urllib.urlencode(params)
        url = url + ('/' if '/' not in url else '') + ('?' if '?' not in url else '&') + params
    elif method == 'POST':
        c.setopt(pycurl.POST, 1)
        if len(params) > 0:
            c.setopt(pycurl.POSTFIELDS, params) #Someone had forgot an 'S'
            c.setopt(pycurl.POSTFIELDSIZE, len(params))
    url = ensure_utf8(url)
    c.setopt(pycurl.URL, url)
    if port > 0 and port < 65536:
        c.setopt(pycurl.PORT, port)

    real_headers = {'User-Agent': 'OSD Lyrics'}
    real_headers.update(headers)
    curl_headers = ['%s:%s' % (k, v) for k, v in real_headers.items()]
    c.setopt(pycurl.HTTPHEADER, curl_headers)

    if proxy is not None and proxy.protocol != 'no':
        if proxy.username != '' and proxy.username is not None:
            proxyvalue = '%s://%s:%s@%s:%d' % (proxy.protocol,
                                               proxy.username,
                                               proxy.password,
                                               proxy.host,
                                               proxy.port)
        else:
            proxyvalue = '%s://%s:%d' % (proxy.protocol,
                                         proxy.host,
                                         proxy.port)
        c.setopt(pycurl.PROXY, proxyvalue)
    else:
        c.setopt(pycurl.PROXY, '')

    c.perform()
    return c.getinfo(pycurl.HTTP_CODE), buf.getvalue()

def ensure_path(path, ignore_file_name=True):
    """ Create directories if necessary.

    This function tries to create directories for `path`. Unlike `os.makedirs`,
    no error will be raised if the leaf directory exists.

    Arguments:
    - `path`: The path.
    - `ignore_file_name`: (optional) If set to `True`, the path to be create will be
       the directory part of `path` which is get from `os.path.dirname(path)`.
       Otherwise `path` is considered to be path to a directory rather than a file.
       The default value is `True`.
    """
    if ignore_file_name:
        path = os.path.dirname(path)

    if os.path.isdir(path):
        return
    os.makedirs(path)

def find_file_in_dirs(filename, dirs, filter_func=None):
    """
    Find file in specified directories. Return a list of full path of found
    files
    Arguments:
    - `filename`: the filename to find
    - `dirs`: a list of directory paths
    - `filter_func`: a filter function that takes the full path of a file
                     and returns a boolean value. If `False` is returned by the
                     function, the path will not be in the returned list.
                     Default value is None
    """
    ret = []
    for dir in dirs:
        path = os.path.join(dir, filename)
        if os.path.isfile(path) and \
                (not callable(filter_func) or filter_func(path)):
            ret.append(path)
    return ret

def cmd_exists(cmd):
    """
    Check if a command exists.
    >>> cmd_exists('ls')
    True
    >>> cmd_exists('cmd_not_likely_exists')
    False
    """
    cmdfiles = find_file_in_dirs(cmd,
                                 os.environ['PATH'].split(':'),
                                 is_exec_file)
    return len(cmdfiles) > 0

def is_exec_file(filepath):
    """
    Check if a file is executable by current user
    """
    try:
        st = os.stat(filepath)
    except Exception:
        return False
    uid = os.getuid()
    gid = os.getgroups()
    if st.st_uid == uid:
        return (st.st_mode & stat.S_IXUSR) != 0
    if st.st_gid in gid:
        return (st.st_mode & stat.S_IXGRP) != 0
    return (st.st_mode & stat.S_IXOTH) != 0

if __name__ == '__main__':
    import doctest
    doctest.testmod()
