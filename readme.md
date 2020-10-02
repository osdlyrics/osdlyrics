edit src/ol_metadata.c to support title parse

install instruction:

sudo apt install osdlyrics
./autogen.sh
./configure --prefix=/tmp PYTHON=/usr/bin/python3
make
sudo make install
sudo cp /tmp/bin/osdlyrics /usr/bin/osdlyrics

build required:(ubuntu20.04)

autoconf automake libtool
libglib2.0-dev
libgtk2.0-dev
libdbus-glib-1-dev
libnotify-dev
intltool
libappindicator-dev

by lakedai 2020/10/02
