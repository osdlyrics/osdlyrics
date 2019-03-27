# OSD Lyrics

Show synced lyrics with your favorite media player on Linux.

## Introduction

OSD Lyrics is a desktop application to view lyrics compatible with various media players. It is not a plugin but a standalone program. OSD Lyrics shows lyrics on your desktop, in the style similar to KaraOK. It also provides another displaying style, in which lyrics scroll from bottom to top. OSD Lyrics can download lyrics from the network automatically.

## License

OSD Lyrics is released under GPL v3. See [LICENSE](LICENSE) for more detail.

## How to use

To use OSD Lyrics, just launch your media player, then launch OSD Lyrics.
OSD Lyrics will detect and connect to the media player.

## How to install

OSD Lyrics is available for Ubuntu in [our PPA](https://launchpad.net/~osd-lyrics/+archive/ubuntu/ppa) and for ArchLinux through [community repo](https://www.archlinux.org/packages/community/x86_64/osdlyrics) and through [AUR](https://aur.archlinux.org/packages/osdlyrics-git).

## Build instructions

### Requirements

On Ubuntu, the following packages need to be installed to compile OSD Lyrics:
- `libappindicator-dev`
- `libdbus-glib-1-dev`

## Cloning repository

You can use git to clone our repository with the latest changes, use the following command:

```
git clone https://github.com/osdlyrics/osdlyrics.git
```

## Building

After cloning the repository and installing dependencies, issue the following commands at the cloned directory to build and install:

```
./autogen.sh
./configure --prefix=/usr PYTHON=/usr/bin/python2
make
sudo make install
```

You can read more at [Building wiki page](https://github.com/osdlyrics/osdlyrics/wiki/Building).

## Troubleshooting

Check [Troubleshooting wiki page](https://github.com/osdlyrics/osdlyrics/wiki/Troubleshooting) for more detailed help.

## Contact us

The official source repository is on Github: https://github.com/osdlyrics/osdlyrics

If there is any feature request, suggestion, or bug, feel free to report them in [issues page](https://github.com/osdlyrics/osdlyrics/issues).

You can contact the developers for fast support through [our discord server](https://discord.gg/anUy3K).

## How to contribute

We adopt [GitHub flow](https://guides.github.com/introduction/flow/index.html) for development, this means you can use GitHub issues, projects and pull requests pages to respectively submit bugs/suggestions, take a look at current roadmap/Kanban and submit changes.

You can read more at [Contributing wiki page](https://github.com/osdlyrics/osdlyrics/wiki/Contributing).

## Acknowledgements

Thanks to all the people who have directly or indirectly helped in the development of this project.
Special thanks to the original author "[Tiger Soldier](mailto:tigersoldi@gmail.com)", first contributors "[Sarlmol Apple](mailto:sarlmolapple@gmail.com)", "[Simply Zhao](mailto:simplyzhao@gmail.com)" and everybody else listed at [contributors page](https://github.com/osdlyrics/osdlyrics/graphs/contributors).
