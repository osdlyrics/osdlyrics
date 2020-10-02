the osdlyrics at the root is the build result(ubuntu20.04)

install instruction:

sudo apt install osdlyrics
./autogen.sh
./configure --prefix=/tmp PYTHON=/usr/bin/python3
make
sudo make install
sudo cp /tmp/bin/osdlyrics /usr/bin/osdlyrics

build required:

autoconf automake libtool
libglib2.0-dev
libgtk2.0-dev
libdbus-glib-1-dev
libnotify-dev
intltool
libappindicator-dev

