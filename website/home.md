# warcog

A free software game engine for RTS-like games

## Overview

## Downloads

## Compiling

Make sure you have at least `gcc 5` or `clang 3.8`. Compiling also requires `make` and `python3`.

```
# install dependencies (debian/ubuntu)

apt-get install libx11-dev libopus-dev libasound2-dev mesa-common-dev

# clone repo

git clone git://warcog.org/warcog
cd warcog
```

The makefile allows for compiling different components of the projet with different options:

```
make [target] [options]
```

where `target` is one of the following:
 `client`
  build the game client, `vulkan=1` to use vulkan
 `server`
  build the game server, `map=path` to use map at `path`
 `chatclient`
  build the chat client (Windows only at the moment)
 `chatserver`
  build the chat server
 `installer`
  build the installer
 `website`
  build the website (markdown to html conversion)

and `options` is a list of `make` variable assignments:
 `fast=1`
  enable optimizations
 `target=value`
  `value` can be `win32` to target Windows
 `python=path`
  use `python` executable at `path`

Some examples:

```
# optimizations

make client fast=1

# vulkan renderer

make client vulkan=1

# cross compilation

make client CC=x86_64-w64-mingw32-gcc target=win32

# server

make server map=warlock fast=1
```

Output files will be in the `bin/` folder and temporary files in the `tmp/` folder.

```
# run server and connect

cd bin

./server &

./client 127.0.0.1 1337
```

## Community

IRC: `irc.warcorg.org`
