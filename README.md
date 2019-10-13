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

Prebuilt versions of Quentier can be downloaded from the following locations:

 * Stable version:
   * Quentier for Windows:
     * [Qt 5.13 32 bit installer](https://github.com/d1vanov/quentier/releases/download/continuous-master/SetupQuentier.0.5.0.Qt.5.13.0.MSVC2017.Win32.exe) (built with MSVC 2017, might require admin rights to install runtime)
     * [Qt 5.13 32 bit portable version](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier-0.5.0-windows-portable-qt513-VS2017_x86.zip) (built with MSVC 2017, doesn't contain runtime which might need to be installed separately)
     * [Qt 5.13 64 bit installer](https://github.com/d1vanov/quentier/releases/download/continuous-master/SetupQuentier.0.5.0.Qt.5.13.0.MSVC2017.x64.exe) (built with MSVC 2017, might require admin rights to install runtime)
     * [Qt 5.10 64 bit portable version](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier-0.5.0-windows-portable-qt513-VS2017_x64.zip) (built with MSVC 2017, doesn't contain runtime which might need to be installed separately)
     * [Qt 5.5 32 bit installer](https://github.com/d1vanov/quentier/releases/download/continuous-master/SetupQuentier.0.5.0.Qt.5.5.1.MinGW.5.3.0.Win32.exe) (built with MinGW)
     * [Qt 5.5 32 bit portable version](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier-0.5.0-windows-portable-qt55-MinGW_x86.zip) (built with MinGW)
   * [Quentier for Mac](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier_mac_x86_64.zip)
   * [Quentier for Linux (AppImage)](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier-master-x86_64.AppImage)
 * Unstable version:
   * Quentier for Windows:
     * [Qt 5.13 32 bit installer](https://github.com/d1vanov/quentier/releases/download/continuous-development/SetupQuentier.0.5.0.Qt.5.13.0.MSVC2017.Win32.exe) (built with MSVC 2017, might require admin rights to install runtime)
     * [Qt 5.13 32 bit portable version](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier-0.5.0-windows-portable-qt513-VS2017_x86.zip) (built with MSVC 2017, doesn't contain runtime which might need to be installed separately)
     * [Qt 5.13 64 bit installer](https://github.com/d1vanov/quentier/releases/download/continuous-development/SetupQuentier.0.5.0.Qt.5.13.0.MSVC2017.x64.exe) (built with MSVC 2017, might require admin rights to install runtime)
     * [Qt 5.13 64 bit portable version](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier-0.5.0-windows-portable-qt513-VS2017_x64.zip) (built with MSVC 2017, doesn't contain runtime which might need to be installed separately)
     * [Qt 5.5 32 bit installer](https://github.com/d1vanov/quentier/releases/download/continuous-development/SetupQuentier.0.4.0.Qt.5.5.1.MinGW.5.3.0.Win32.exe) (built with MinGW)
     * [Qt 5.5 32 bit portable version](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier-0.4.0-windows-portable-qt55-MinGW_x86.zip) (built with MinGW)
   * [Quentier for Mac](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier_mac_x86_64.zip)
   * [Quentier for Linux (AppImage)](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier-development-x86_64.AppImage)

Stable versions correspond to builds from `master` branch, unstable versions correspond to builds from `development` branch. New stuff first appears in unstable versions but for this reason they are also prone to temporary breakages.

For users of **Ubuntu** and derivatives there's a [PPA repository](https://launchpad.net/~d1vanov/+archive/ubuntu/quentier-stable) from where it is easy to install Quentier:
```
sudo add-apt-repository ppa:d1vanov/quentier-stable
sudo apt-get update
sudo apt-get install quentier-qt5
```
Alternatively, if you run Qt4-based desktop environment (KDE 4), you might want to install Qt4-based Quentier instead:
```
sudo apt-get install quentier-qt4
```
This PPA is for stable Quentier builds from master branch. Alternatively, the PPA for unstable version of Quentier is [the following](https://launchpad.net/~d1vanov/+archive/ubuntu/quentier-development):
```
sudo add-apt-repository ppa:d1vanov/quentier-development
sudo apt-get update
sudo apt-get install quentier-qt5
```
or
```
sudo apt-get install quentier-qt4
```

For users of other Linux distributions there are also prepared repositories with native packages:

 * Stable versions:
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-master&package=quentier) for Fedora, OpenSUSE, and Arch Linux repositories
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-master&package=quentier-qt5) for Debian repositories
 * Unstable versions:
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-development&package=quentier) for Fedora, OpenSUSE and Arch Linux repositories
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-development&package=quentier-qt5) for Debian repositories

## How to build from source

See the [building/installation guide](INSTALL.md).

## How to contribute

Contributions to Quentier are warmly welcome! Please see the [contribution guide](CONTRIBUTING.md) to see how you can contribute.
