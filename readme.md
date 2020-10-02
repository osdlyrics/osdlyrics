## search text help
- if no tag, find title / artist from meta title
- support meta title formats:
- %n.%p-%t, %n.%t--%p, %n.%t, %p-%t, %t--%p, %t

- changed src files:
	修改：     src/ol_metadata.c
	修改：     src/ol_metadata.h
	修改：     src/ol_search_dialog.c

## install instruction:
	sudo apt install osdlyrics
	./autogen.sh
	./configure --prefix=/tmp PYTHON=/usr/bin/python3
	make
	sudo make install
	sudo cp /tmp/bin/osdlyrics /usr/bin/osdlyrics

## build required:(ubuntu20.04)

- autoconf automake libtool
- libglib2.0-dev
- libgtk2.0-dev
- libdbus-glib-1-dev
- libnotify-dev
- intltool
- libappindicator-dev

*by lakedai 2020/10/02*
