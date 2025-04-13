Quentier
========

<img src="https://d1vanov.github.io/quentier/apple-touch-icon.png">

**Cross-platform desktop Evernote client**

![master](https://github.com/d1vanov/quentier/actions/workflows/build_and_deploy.yml/badge.svg?branch=master)

## What's this

Quentier is a cross-platform desktop note taking app capable of working as Evernote client. You can also use Quentier
for local notes without any connection to Evernote and synchronization.

Quentier is free software distributed under the terms of GPL v.3.

<img src="https://d1vanov.github.io/quentier/Quentier.gif">

Quentier is written in C++ using Qt framework and is built on top of [libquentier](http://github.com/d1vanov/libquentier) library.

Read the project's [blog](https://d1vanov.github.io/quentier) and [wiki](https://github.com/d1vanov/quentier/wiki) for more details.

## Download / Install

There are two release channels: stable and unstable one. Stable versions correspond to builds from `master` branch, unstable versions correspond to builds from `development` branch. New stuff first appears in unstable versions but for this reason they are also prone to temporary breakages.

Prebuilt versions of Quentier are available on GitHub releases:

 * Stable version:
   * Quentier for Windows:
     * [Installer](https://github.com/d1vanov/quentier/releases/download/continuous-master/SetupQuentier.0.5.0.Qt.5.15.2.MSVC2019.x64.exe) (might require admin rights to install VC++ runtime)
     * [Portable version](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier-windows-portable-x64.zip) (doesn't contain VC++ runtime which might need to be installed separately)
   * [Quentier for Mac](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier_mac_x86_64.zip)
   * [Quentier for Linux (AppImage)](https://github.com/d1vanov/quentier/releases/download/continuous-master/Quentier-master-x86_64.AppImage)
 * Unstable version:
   * Quentier for Windows:
     * [Installer](https://github.com/d1vanov/quentier/releases/download/continuous-development/SetupQuentier.0.6.0.Qt.5.15.2.MSVC2019.x64.exe) (might require admin rights to install VC++ runtime)
     * [Portable version](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier-windows-portable-qt5-x64.zip) (doesn't contain VC++ runtime which might need to be installed separately)
   * [Quentier for Mac](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier_mac_x86_64.zip)
   * [Quentier for Linux (AppImage)](https://github.com/d1vanov/quentier/releases/download/continuous-development/Quentier-development-x86_64.AppImage)


For Linux users there are also package repositories:

PPA repositories for **Ubuntu** and derivatives:

[Stable version PPA](https://launchpad.net/~d1vanov/+archive/ubuntu/quentier-stable):
```
sudo add-apt-repository ppa:d1vanov/quentier-stable
sudo apt-get update
sudo apt-get install quentier-qt5
```

[Unstable version PPA](https://launchpad.net/~d1vanov/+archive/ubuntu/quentier-development):
```
sudo add-apt-repository ppa:d1vanov/quentier-development
sudo apt-get update
sudo apt-get install quentier-qt5
```

For other Linux distributions:

 * Stable versions:
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-master&package=quentier) for Fedora and OpenSUSE repositories
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-master&package=quentier-qt5) for Debian repositories
   * Use [this repository](https://download.opensuse.org/repositories/home:/d1vanov:/quentier-master/Arch_Community/x86_64/) for Arch Linux
 * Unstable versions:
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-development&package=quentier) for Fedora and OpenSUSE repositories
   * See [this page](https://software.opensuse.org//download.html?project=home%3Ad1vanov%3Aquentier-development&package=quentier-qt5) for Debian repositories
   * Use [this repository](https://download.opensuse.org/repositories/home:/d1vanov:/quentier-development/Arch_Community/x86_64/) for Arch Linux

## How to build from source

See the [building/installation guide](INSTALL.md).

## How to contribute

Contributions to Quentier are warmly welcome! Please see the [contribution guide](CONTRIBUTING.md) to see how you can contribute.
