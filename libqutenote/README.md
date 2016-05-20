libqutenote
===========

**Set of APIs for feature rich desktop clients for Evernote service for Qt**

## What's this

This library presents the set of APIs implementing different pieces of functionality required for any application working as a feature rich desktop client for Evernote service. The APIs are implemented in Qt/C++. The most important and useful components of the library are the following:

* Local storage
* Synchronization
* Note editor
* Set of types wrapping the types provided by [QEverCloud](https://github.com/d1vanov/QEverCloud) library and adding details essential for using them along with the other library components

## Compatibility

The library supports both Qt4 and Qt5 branches. In particular, the supported Qt4 version requires Qt 4.8.6+ and Qt5 version requires Qt 5.5.1+ version.

### C++11 features

The major part of the library code is written in C++98 style while also using some features of C++11 standard supported by some older compilers. In particular, compilers as old as gcc-4.4 and MVSC-2010 are capable of building libqutenote. 

### Note editor backends

The note editor component of the library currently uses two different backends for Qt4 and Qt5 branches: QtWebKit for the former one and QWebEngine for the latter one. 
