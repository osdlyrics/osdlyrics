===============================
 OSD Lyrics DBus specification
===============================

Bus Names
=========

The daemon of OSD Lyrics uses ``org.osdlyrics.Daemon`` as well-known bus name.

Data Structures
===============

Lyrics Data
-----------
``aa{sv}``

Lyrics are repesented as array of dicts. Each dict in array represent a line of lyric. dicts are in ascending order by timestamp.

Fields of lyrics dict are:

 - id: uint32. Starts from 0 and increases according to timestamp.
 - timestamp: int64. The start time of the lyric text in microseconds.
 - text: string. The lyric text itself.

Lyric URI
----------
``s``

The path of an LRC file. It is in url format. Currently available schemas are:

 - `file:` The lyrics are stored in local file system. The path of the lyrics is the path of the URI. Example: file:///home/osdlyrics/track1.lrc
 - `tag:` The lyrics are stored in ID3 tag of the track. The path of the music file to store the ID3 tag is specified in the path of the URI. Example: tag:///home/osdlyrics/track1.ogg
 - `none:` The track is assigned not to show any lyrics. Example: none:

For compatability reasons, an empty string is considered to identical to `none:`.

Player Info
--------
``a{sv}``

An dict of information of supported player application. Avaliable fields are:

 - name: string. The name used to identify the player in OSD Lyrics. It SHOULD be composed with alphabet, underscore, or digits only.
 - appname: string. The display name of the player application.
 - binname: string. The name of executable file of the player applicatoin.
 - cmd: string. The command line to launch the player.
 - icon: string. *(optional)* The name of icon of the player application. May be empty if it is a cli player.

Metadata
--------
``a{sv}``

Use the format described in `MPRIS Metadata <http://xmms2.org/wiki/MPRIS_Metadata>`_, with the following fields:

 - title: (string)
 - artist: (string)
 - album: (string)
 - location: (string)
 - tracknumber: (string)
 - arturl: (string)
 - time: (uint32) The duration in seconds
 - mtime: (uint32) The duration in milliseconds

Search Result
-------------
``a{sv}``

 - source: (string) The id of lyric source plugin that provides the lyric
 - title: (string) The title of the matched lyric
 - artist: (string) The artist of the matched lyric
 - album: (string) The album of the matched lyric
 - sourceid: (string) The ID of the source that provides this result. Lyric Source implementations MUST set this value correctly.
 - downloadinfo: (variable) The private data provided by the lyric source. It's used to download the lyric. Lyric sources can set any value to it as long as the source plugin can figure out how to download the lyric with this value. Usually it's a URL string of the lyric content. GUI clients should pass the value to <TODO> method.

Lyric Source
------------
``a{sv}``

 - id: (string) The id of lyric source plugin
 - name: (string) The localized name of the lyric source plugin
 - enabled: (boolean) True if the source is enabled in config.

Interfaces
==========

The Daemon
----------

The object path is ``/org/osdlyrics/Daemon``. The interface is ``org.osdlyrics.Daemon``.

Methods
~~~~~~~

Hello(s:client_bus_name) -> None
  Notifies that a client has connected to the daemon. The daemon watches all the bus names of clients that says hello to it. If all clients disappeared, which means the bus names of clients vanished, the daemon quits automatically.

  A valid bus name of the client should be in the format of ``org.osdlyrics.Client.ClientName``.

  It is possible to not notify the daemon. However, the daemon may quit at any time, even if the client is still running.

GetVersion() -> s
  Returns the current version of the daemon.

Quit() -> None
  Quits the daemon

Player Controlling
------------------

OSD Lyrics follows `MPRIS1 <http://xmms2.org/wiki/MPRIS>`_ and `MPRIS2 specification <http://www.mpris.org/2.1/spec/>`_ for controlling players.

OSD Lyrics uses the bus name ``org.mpris.MediaPlayer2.osdlyrics`` as an alias name according to the specification of MPRIS2.

Player Support
--------------

These are features about players not covered by MPRIS2.

The object path is ``/org/osdlyrics/Player``.

The interface is ``org.osdlyrics.Player``

Methods
~~~~~~~

ListSupportedPlayers() -> aa{sv}
  Gets an array of infomation of supported players.

  The returned value is an array of dicts. The fields of the dict is described in `Player Info`_.

ListActivatablePlayers() -> aa{sv}
  Similar to ``GetSupportedPlayers``, but the array contains the supported players which are installed in the computer only.

GetCurrentPlayer() -> b, a{sv}
  Gets the infomation of player currently connected to.

  If no supported player is running, the first returned value is False. Otherwise the first returned value is True, and the second value is the infomation of the player in the format described in `Player Info`_.

Signals
~~~~~~~
PlayerLost()
  Emit when the currently connected player quits.

PlayerConnected(a{sv})
  Emit when a support player is launched and connected as current player.

  player_info(a{sv}): The info of connected player. The format is described in `Player Info`_

Lyrics
------

The object path for lyrics manipulation is ``/org/osdlyrics/Lyrics``.

The interface is ``org.osdlyrics.Lyrics``.

Methods
~~~~~~~

GetLyrics(a{sv}:metadata) -> b, s, a{ss}, aa{sv}
  Gets the lyircs of specified metadata.

  Return values:

  - ``found(b)``: Whether the lyrics file is found.
  - ``uri(s)``: The URI of the lyrics file. See `Lyrics URI`_ for more details. If no lyrics found, the uri is an empty string.
  - ``attributes(a{ss})``: The key-value attributes in the LRC file, such like [title:The title].
  - ``content(aa{sv})``: The content of the lyrics. See `Lyrics Data`_ for more details. If no lyrics found, an empty array will be returned.

GetCurrentLyrics() -> b, s, a{ss}, aa{sv}
  Similar to GetLyrics. Returns the lyrics of the current playing track.

GetRawLyrics(a{sv}:metadata) -> b, s, s
  Gets the content of LRC file of specified metadata.

  Return values:

  - ``found(b)``: Whether the lyrics file is found.
  - ``uri(s)``: The URI of the lyrics file. See `Lyrics URI`_ for more details. If no lyrics found, the uri is an empty string.
  - ``content(s)``: The content of the LRC file. If no lyrics found, an empty string will be returned.

GetCurrentRawLyrics() -> b, s, s
  Similar to GetRawLyrics.

  Returns the content of LRC file of current playing track.

SetLyricContent(a{sv}:metadata, ay:content) -> s
  Sets the lyrics of specified metadata by content of LRC file.

  The content is a byte string rather than utf-8 string so that the content can be encoded in other charsets.

  Returns the URI of assigned lyrics. The URI follows the format described in `Lyric URI`_. If the given metadata cannot be expended to a valid path, or errors raised when saving the content to the file, an empty string is returned and the lyrics to the metadata is not changed.

AssignLyricFile(a{sv}:metadata, s:uri) -> nothing
  Assigns an LRC file to given metadata. The ``uri`` should follow the format described in `Lyric URI`_.

SetOffset(s:uri, i:offset_ms)
  Sets the offset of an LRC file. The ``uri`` should be a valid lyrics URI described in `Lyric URI`_. The ``offset`` is in milliseconds. Errors will be raise as exceptions.

Signals
~~~~~~~

CurrentLyricsChanged()
  The current lyrics is changed by ``SetLyricContent`` or ``AssignLyricFile``, or lyrics downloaded. This signal will be emitted only when the lyrics of the SAME track is changed. If the track is changed, the signal will not be emitted.

Search/download lyrics
----------------------------

The object for lyric searching/download in ``/org/osdlyrics/LyricSource``. This object implements ``org.osdlyrics.LyricSource`` interface.

Methods
-------

ListSources() -> aa{sv}: lyricSources
  List all available lyric sources.

  Returns:

  - ``lyricsSources``: An array of `Lyric Source_`. The lyric sources in the array are in the order of the priority defined in the config item ``Download/download-engine``

Search(a{sv}:metadata, as:sources) -> int32:ticket
  Search lyrics for a track with given metadata. Returns an integer to identify the search task.

  Parameters:

  - ``metadata``: The metadata of the track to be searched for. The metadata SHOULD contain at least ``title`` or ``uri``.
  - ``sources``: Array of IDs of lyric sources. The elements must be the ``id`` of `Lyric Source_` returned by  ``ListSources``. If ``sources`` is an empty array, the available sources will be chosen from user config. Search request will be send to the first lyric source in the array, then the second if the first one returns nothing, and so on. When the search request is sent to a source, a ``SearchStarted`` signal will be emitted, with the name of the source. When search is complete or failed, a ``SearchStatusChanged`` signal will be emitted.

  Returns:

  - ``ticket``: An integer to identify the search task. The ticket can be used in ``CancelSearch`` or ``SearchStatusChanged``.

CancelSearch(int32:ticket) ->nothing
  Cancel a search task.

  Parameter:

  - ``ticket``: The ticket to identify the search task to be cancelled.

Download(s:source, v:downloaddata) -> int32:ticket
  Download lyric content.

  Parameters:

  - ``source``: The id of lyric source to download from. Id MUST be the same as the ``source`` field in `Lyric Source_`.
  - ``downloadinfo``: The ``downloadinfo`` field in `Lyric Source_`. ``downloadinfo`` and ``source`` must be taken from the same `Lyric Source_`.

CancelDownload(int32:ticket) ->nothing
  Cancel a download task.

  Parameter:

  - ``ticket``: The ticket to identify the download task to be cancelled.

Signals
-------

SearchStarted(int32:ticket, s:sourceid, s:sourcename)
  Notify that the OSD Lyrics start to search lyrics from a source.

  For each search task, there may be more than one ``SearchStarted`` signal. Searching request will be sent to the first source of the ``sources`` parameter in ``Search`` call. If the first source fail to search or returns nothing, the search request is sent to the second source and a second ``SearchStarted`` signal is emitted, and so on.

  Parameters:

  - ``ticket``: The ticket to identify the search task.
  - ``sourceid``: The id of the lyric source to start with.
  - ``sourcename``: The name of the source.

SearchComplete(int32:ticket, int32:status, aa{sv}:results)
  Emit when a search request is finished, cancelled or failed.

  Note that if all source returns nothing, the status is finished with empty ``results``, not failed.

  Parameters:

  - ``ticket``: The ticket to identify the search task.
  - ``status``: The status of the task. MUST be one of the following:
    - 0: Search finished. Search result is saved in ``results``.
    - 1: Search is cancelled. ``results`` SHOULD be an empty array.
    - 2: Search failed. ``results`` SHOULD be an empty array.
  - ``results``: An array of `Search Result_`, the results returned from sources.

DownloadComplete(int32:ticket, int32:status, ay:content)
  Emit when a download task is finished, cancelled or failed.

  Parameters:

  - ``ticket``: The ticket to identify the search task.
  - ``status``: The status of the task. MUST be one of the following:
    - 0: Download finished. If the lyric downloaded assiged to the currently playing track, clients SHOULD receive a ``CurrentLyricsChanged`` signal from ``org.osdlyrics.Lyrics`` interface.
    - 1: Search is cancelled by user.
    - 2: Search is failed.
  - ``content``: The content of the lyric

Configure Service
=================
The well-known bus name of configure module is ``org.osdlyrics.Config``

The object path of configuration is ``/org/osdlyrics/Config``.

The interface is ``org.osdlyrics.config``.

The name of configure options
-----------------------------
All the name used in configure options MUST be in the format of "group_name/options_name". For example, the visibility of the OSD Window when the player is stopped should be configured in "OSD/visible_when_stopped".

Methods
-------

GetInt(s:name) -> int32
  Gets an int32 in config. If the value of name does not exists, ``org.osdlyrics.Error.ValueNotExist`` exception will be raised.

SetInt(s:name, int32:value)
  Sets an int32 value.

GetString(s:name) -> s
  Gets a string. If the value of name does not exists, ``org.osdlyrics.Error.ValueNotExist`` exception will be raised.

SetString(s:name, s:value)
  Sets a string value.

GetBool(s:name) -> b
  Gets a boolean value. If the value of name does not exists, ``org.osdlyrics.Error.ValueNotExist`` exception will be raised.

SetBool(s:name, b:value)
  Sets a boolean value.

GetDouble(s:name) -> d
  Gets a double value. If the value of name does not exists, ``org.osdlyrics.Error.ValueNotExist`` exception will be raised.

SetDouble(s:name, d:value)
  Sets a double value.

GetStringList(s:name) -> as
  Gets an array of strings. If the value of name does not exists, ``org.osdlyrics.Error.ValueNotExist`` exception will be raised.

SetStringList(s:name, as:value)
  Sets an array of string.

SetDefaultValues(a{sv}:values)
  Sets a set of default values. The existing values will not be overwrited, only
values that not exists will be set.

  Parameters:

  - `values`: a dictionary, the key is the name of the value, and the value is the
              value itself. The value should be one of the following types: b, i, d,
              s, as, which are boolean, integer, double, string, string list,
              respectively.

Signals
-------

ValueChanged(as:name_list)
  Emit when one or more config value has been changed. ``name_list`` is a list of names of changed values.

Exceptions
----------

org.osdlyrics.Error.ValueNotExistError
  If the name does not exist when calling Get series methods, a ``org.osdlyrics.Error.ValueNotExist`` will be raised.

org.osdlyrics.Error.MalformedKeyError
  If the key does not follow the "group/name" format, a ``org.osdlyrics.Error.MalformedKey``  will be raised.

Player Proxy
============

A player proxy is a client to support one or more players.

A player proxy MUST have a unique name, like ``Mpris`` or ``Exaile03``. The well-known bus name and object path MUST be of the form ``org.osdlyrics.PlayerProxy.proxyname`` and ``/org/osdlyrics/PlayerProxy/proxyname``, where ``proxyname`` is the unique name.

For instance, a player proxy of MPRIS2 may have a unique name ``Mpris2``, and provides the bus name ``org.osdlyrics.PlayerProxy.Mpris2`` with object path ``/org/osdlyrics/PlayerProxy/Mpris2``.

The interface of player proxy is ``org.osdlyrics.PlayerProxy``

Methods
-------

ListActivePlayers() -> aa{sv}
  Lists supported players that are already running.

  Returns an array of dict. The dict represents the information of a player described in `Player Info`_.

ListActivatablePlayers() -> aa{sv}
  Lists supported players installed on the system.

  Returns an array of dict. The dict represents the information of a player described in `Player Info`_.

ListSupportedPlayers() -> aa{sv}
  Lists all supported players which can be launched on system.

ConnectPlayer(s:player_name) -> o
  Connect to an active player. The player proxy SHOULD create an dbus object with the path of ``/org/osdlyrics/PlayerProxy/proxyname/player_name``. The ``player_name`` is the ``name`` field described in `Player Info`_.

  The path of created object is returned. The created player object MUST implement interfaces described in `Player Instance`_.

  If the player cannot be create with ``player_name``, a ``org.osdlyrics.Error.ConnectPlayerError`` SHOULD be raised.

Signals
-------

PlayerLost(s)
  The player of name s is lost

Exceptions
----------

org.osdlyrics.Error.ConnectPlayerError
  Raised when ``ConnectPlayer`` fails.

Player Instance
===============

Player instances are created by `ConnectPlayer` method of `Player Proxy`_. A player instance is used to control a single player.

Player instance MUST implement `org.mpris.MediaPlayer2.Player<http://specifications.freedesktop.org/mpris-spec/latest/Player_Node.html>`_ interface of `MPRIS2<http://specifications.freedesktop.org/mpris-spec/latest/>`_ specification. The object path MUST be the path returned by `ConnectPlayer` method of `Player Proxy`_ instead of `/org/mpris/MediaPlayer2`.

Lyric Source Plugins
==============================

To write a plugin to support a new lyric source to download or upload(optional), the plugins should follow the specification here.

A lyric source plugin Must have a unique name, like ``ttplayer``. The well-known bus name should be ``org.osdlyrics.LyricSourcePlugin.<pluginname>``. The object path should be ``/org/osdlyrics/LyricSourcePlugin/<pluginname>``. ``<pluginname>`` here stands for the unique name of the plugin.

All lyric source plugin should implement ``org.osdlyrics.LyricSourcePlugin`` interface. The interface is defined below:

Methods
-------

Search(a{sv}:metadata) -> int32: ticket
  Search lyrics for a track with given metadata.

  Parameters:

  - `metadata`: The metadata of the track to be searched. The metadata should contain at least title or uri information. Otherwise searching will fail instantly.

  Return:

  - `ticket`: An unique integer to identify the search task in ``SearchComplete`` signal and ``CancelSearch`` method.

CancelSearch(int32: ticket) -> nothing
  Cancel a search task.

  Parameters:

  - `ticket`: The ticket returned by ``Search``.

Download(v: downloadinfo) -> int32: ticket
  Download lyric with given downloadinfo.

  Parameters:

  - `downloadinfo`: The information to download the lyric content. This information should be the value of ``downloadinfo`` in `Search Result_`.

  Return:

  - `ticket`: An unique integer to identify the download task. This value should be used in ``DownloadComplete`` signal and ``CancelDownload`` method.

CancelDownload(int32: ticket) -> nothing
  Cancel a download task with given ticket.

  Parameters:

  - `ticket`: The ticket returned by ``Download``

Properties
----------

Name: string, readonly
  The name of the lyric source. It's used to show to users, not the unique name. The name should be localized by the lyric source plugin.


Signals
-------

SearchComplete(i: ticket, i: status, aa{sv}: results)
  Emit when a search task is succeeded, canceled or failed.

  Parameters:

  - `ticket`: The ticket to identify the search task. This MUST be the same ticket that ``Search`` returns for this task.
  - `status`: The status of the search task. MUST be one of the following:
    - 0: Search succeed, the `results` value MUST be the search results. Even there is no matched results, the `status` should be succeed, not failure
    - 1: Search task is canceled. The `result` value SHOULD be an empty array.
    - 2: Search fail. The `result` value SHOULD be an empty array.
  - `results`: Array of `Search Result_`. It SHOULD be set if `status` is succeed (0), otherwise it SHOULD be an empty array.

DownloadComplete(i: ticket, i: status, ay: content)
  Emit when a download task is succeeded, canceled or failed.

  Parameters:

  - `ticket`: The ticket to identify the download task. This MUST be the same ticket that ``Download`` returns for this task.
  - `status`: The status of the download task. MUST be one of the following:
    - 0: Search succeed, the `results` value MUST be the lyric content.
    - 1: Search task is canceled. The `result` value SHOULD be an empty byte array.
    - 2: Search fail. The `result` value SHOULD be an empty byte array.
