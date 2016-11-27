# -*- coding: utf-8 -*-
#
# Copyright (C) 2012 Tiger Soldier <tigersoldi@gmail.com>
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

import re
import os
import os.path
import string

ROOTMAKEFILEAM = r"""SUBDIRS = src
"""

MAKEFILEAM = r"""${name}_PYTHON = ${name}.py

${name}dir = $$(pkglibdir)/lyricsources/${name}


servicedir = $$(datadir)/dbus-1/services
service_in_files = org.osdlyrics.LyricSourcePlugin.${name}.service.in
service_DATA = $$(service_in_files:.service.in=.service)

EXTRA_DIST = \
	$$(service_in_files) \
	$$(NULL)

$$(service_DATA): $$(service_in_files)
	@sed -e "s|\@pkglibdir\@|$$(pkglibdir)|" -e "s|\@PYTHON\@|$$(PYTHON)|" $$< > $$@

CLEANFILES = \
	org.osdlyrics.LyricSourcePlugin.${name}.service \
	$$(NULL)
"""

SERVICE = r"""[D-BUS Service]
Name=org.osdlyrics.LyricSourcePlugin.${name}
Exec=@PYTHON@ @pkglibdir@/lyricsources/${name}/${name}.py
"""

PYTHON = r"""# -*- coding: utf-8 -*-

class ${capsname}Source(BaseLyricSourcePlugin):
    def __init__(self):
        
        BaseLyricSourcePlugin.__init__(self, id='${name}', name='${name}')

    def do_search(self, metadata):
        # return list of SearchResult
        # you can make use of utils.http_download
        #
        # example:
        status, content = http_download(url='http://foo.bar/foobar'
                                        params={param1='foo', param2='bar'},
                                        proxy=get_proxy_settings(config=self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status, '')
        # now do something with content
        return [SearchResult(title='title',
                             artist='artist',
                             album='album',
                             sourceid=self.id,
                             downloadinfo='http://foo.bar/download?id=1')]

    def do_download(self, downloadinfo):
        # return a string
        # downloadinfo is what you set in SearchResult
        if not isinstance(downloadinfo, str) and \
                not isinstance(downloadinfo, unicode):
            raise TypeError('Expect the downloadinfo as a string of url, but got type ',
                            type(downloadinfo))
        status, content = http_download(url=downloadinfo,
                                        proxy=get_proxy_settings(self.config_proxy))
        if status < 200 or status >= 400:
            raise httplib.HTTPException(status, '')
        return content

if __name__ == '__main__':
    ${name} = ${capsname}Source()
    ${name}._app.run()
"""

def input_name():
    prompt = 'Input the lyric source name with only lower-case alphabets and numbers:\n'
    while True:
        name = raw_input(prompt).strip().lower()
        if not re.match(r'[a-z][a-z0-9]*$', name):
            prompt = 'Invalid name. Name must contain only lower-case alphabets and numbers.\nName:'
        else:
            break
    return name

def input_boolean(prompt, default_value):
    prompt += ' [Y/n]?' if default_value == True else ' [y/N]'
    value = raw_input(prompt)
    if value.lower() == 'y':
        return True
    elif value.lower() == 'n':
        return False
    else:
        return default_value == True

def create_file(template, path, name, params):
    content = string.Template(template).substitute(params)
    f = open(os.path.join(path, name), 'w')
    f.write(content)
    f.close()

def main():
    name = input_name()
    have_am = input_boolean('Generate Makefile.am', True)
    have_subdir = input_boolean('Create source files in src subdirectory', False)
    rootpath = name
    srcpath = name if not have_subdir else name + '/src'
    if not os.path.isdir(srcpath):
        os.makedirs(srcpath)
    params = {
        'name': name,
        'capsname': name.capitalize()
        }
    create_file(PYTHON, srcpath, name + '.py', params)
    create_file(SERVICE, srcpath, 'org.osdlyrics.LyricSourcePlugin.' + name + '.service.in', params)
    if have_am:
        create_file(MAKEFILEAM, srcpath, 'Makefile.am', params)
        if have_subdir:
            create_file(ROOTMAKEFILEAM, rootpath, 'Makefile.am', params)
    print 'Done'

if __name__ == '__main__':
    main()
