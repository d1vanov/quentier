Quentier
========

**Cross-platform desktop Evernote client**

Travis CI (Linux, OS X): [![Build Status](https://travis-ci.org/d1vanov/quentier.svg?branch=master)](https://travis-ci.org/d1vanov/quentier)

AppVeyor CI (Windows): [![Build status](https://ci.appveyor.com/api/projects/status/0o2ro87sw1wm3ama/branch/master?svg=true)](https://ci.appveyor.com/project/d1vanov/quentier)

## What's this

Quentier is a cross-platform desktop note taking app capable of working as Evernote client. You can also use Quentier
for local notes without any connection to Evernote and synchronization.

Quentier is free software distributed under the terms of GPL v.3.

<img src="https://d1vanov.github.io/quentier/Quentier.gif">

At the moment Quentier is in public alpha state and is not yet intended for production usage. Currently it's targeted
at software enthusiasts, developers and people who like tinkering with fresh stuff.

Quentier is written in C++ using Qt framework and is built on top of [libquentier](http://github.com/d1vanov/libquentier) library.

Read the project's [blog](https://d1vanov.github.io/quentier) and [wiki](https://github.com/d1vanov/quentier/wiki) for more details.

## Download / Install

See [releases](https://github.com/d1vanov/quentier/releases) page with available releases and downloads for Linux (AppImage), Windows and OS X / macOS.
Note that **Continuous-master** is the latest somewhat stable version, **Continuous-development** is the latest development one.

For users of **Debian/Ubuntu** and derivatives there's a [PPA repository](https://launchpad.net/~d1vanov/+archive/ubuntu/quentier-stable) from where it is easy to install Quentier:
```
sudo add-apt-repository ppa:d1vanov/quentier-stable
sudo apt-get update
sudo apt-get install quentier-qt5
```
Alternatively, if you run Qt4-based desktop environment (KDE 4), you might want to install Qt4-based Quentier instead:
```
sudo apt-get install quentier-qt4
```
The PPA is for daily Quentier builds from master branch. They are considered relatively stable but might lack certain newest developed functionality which has not yet been merged to master branch. If you prefer, you can use the [development PPA repository](https://launchpad.net/~d1vanov/+archive/ubuntu/quentier-development) instead of the stable one.

For **Fedora** users there's the unofficial Quentier [copr repository](https://copr.fedorainfracloud.org/coprs/eclipseo/quentier/) from which Quentier can be installed as simply as
```
sudo dnf copr enable eclipseo/quentier
sudo dnf install quentier
```

## How to build from source

See the [building/installation guide](INSTALL.md).

## How to contribute

Contributions to Quentier are warmly welcome! Please see the [contribution guide](CONTRIBUTING.md) to see how you can contribute.
