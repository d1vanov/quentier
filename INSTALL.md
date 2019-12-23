**Building and installation guide**

## Compatibility

Quentier works on Linux, OS X / macOS and Windows. It can be built with virtually any version of Qt framework,
starting from Qt 4.8.6 and up to the latest and greatest Qt 5.x. The major part of Quentier is written in C++98 style
with a few features of C++11 standard which are supported by older compilers. As a result, Quentier can be built
by as old compilers as gcc-4.5, Visual C++ 2010.

Even though Quentier is cross-platform, most development and testing currently occurs on Linux and OS X / macOS
so things might occasionally break on Windows platform.

## Dependencies

Quentier itself depends on just a few Qt modules:
 * For Qt4: QtCore, QtGui
 * For Qt5: Qt5Core, Qt5Gui, Qt5Widgets, Qt5LinguistTools

However, Quentier also depends on several libraries some of which have their own requirements for Qt modules.
Here are the libraries Quentier depends on:
 * [libquentier](http://github.com/d1vanov/libquentier)
 * [QEverCloud](https://github.com/d1vanov/QEverCloud)
 * Boost program options (>= 1.38)
 * For Qt4 buidls only: [qt4-mimetypes](https://github.com/d1vanov/qt4-mimetypes)
 * Optional although recommended dependency: [Google breakpad](https://chromium.googlesource.com/breakpad/breakpad)

Although it is theoretically possible to use different Qt versions to build Quentier and its dependencies, it is highly
not recommended as it can cause all sort of building and/or runtime issues.

See [the detailed guide](https://github.com/d1vanov/quentier/wiki/Building-and-installation-of-Quentier's-dependencies) on building and installing the dependencies.

## Building

Quentier uses [CMake](https://cmake.org) build system to find all the necessary libraries and to generate makefiles
or IDE project files. Prior to building Quentier you should build and install all of its dependencies listed above.

Here are the basic steps of building Quentier (on Linux and OS X / macOS):
```
cd <...path to cloned Quentier repository...>
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=<...where to install the built app...> ../
make
make install
```

On Windows the `cmake` step is usually more convenient to do using GUI version of `CMake`. For build instead of `make`
it is convenient to call `cmake --build .` command; if target specification is required, it can be done like this:
`cmake --build . --target install`.

The Qt version being searched/used by default is Qt5. If you want to build against Qt4, use `BUILD_WITH_QT4` `CMake` option:
```
cmake -DBUILD_WITH_QT4=YES <...>
```

If you installed Quentier's or libquentier's dependencies into non-standard locations, the `cmake` step
from the above list might fail to find some library. You can give `CMake` some hints where to find the dependencies:

For Qt4:
```
cmake -Dqt4-mimetypes_DIR=<...path to qt4-mimetypes installation folder...>/lib/cmake/qt4-mimetypes \
      -DLIBXML2_INCLUDE_DIR=<...path to libxml2 include directory...> \
      -DLIBXML2_LIBRARIES=<...path to libxml2 library...> \
      -DOPENSSL_ROOT_DIR=<...path to the OpenSSL installation prefix...> \
      -DBOOST_ROOT=<...path to boost installation prefix...> \
      -DQtKeychain_DIR=<...path to QtKeychain cmake dir within the installation prefix...> \
      -DQEverCloud-qt4_DIR=<...path to Qt4 QEverCloud installation...>/lib/cmake/QEverCloud-qt4 \
      -DTIDY_HTML5_INCLUDE_PATH=<...path to tidy-html5 include directory...> \
      -DTIDY_HTML5_LIB=<...path to tidy-html5 library...> \
      -DLibquentier-qt4_DIR=<...path to Qt4 libquentier installation...>/lib/cmake/Libquentier-qt4 \
      -DBREAKPAD_ROOT=<...path to Google breakpad installation prefix...> \
      -DCMAKE_INSTALL_PREFIX=<...where to install the built app...> ../
```
For Qt5:
```
cmake -DLIBXML2_INCLUDE_DIR=<...path to libxml2 include directory...> \
      -DLIBXML2_LIBRARIES=<...path to libxml2 library...> \
      -DOPENSSL_ROOT_DIR=<...path to the OpenSSL installation prefix...> \
      -DBOOST_ROOT=<...path to boost installation prefix...> \
      -DQt5Keychain_DIR=<...path to Qt5 QtKeychain installation...>/lib/cmake/Qt5Keychain \
      -DQEverCloud-qt5_DIR=<...path to Qt5 QEverCloud installation...>/lib/cmake/QEverCloud-qt5 \
      -DTIDY_HTML5_INCLUDE_PATH=<...path to tidy-html5 include directory...> \
      -DTIDY_HTML5_LIB=<...path to tidy-html5 library...> \
      -DLibquentier-qt5_DIR=<...path to Qt5 libquentier installation...>/lib/cmake/Libquentier-qt5 \
      -DBREAKPAD_ROOT=<...path to Google breakpad installation prefix...> \
      -DCMAKE_INSTALL_PREFIX=<...where to install the built app...> ../
```

### Running tests

Quentier comes with a set of self tests which you are encouraged to run if you build Quentier yourself. Obviously,
if tests don't pass, something is wrong. The tests can be run by making `test` or `check` target - the latter one
provides more explicit output than the former one. So the available options to run the tests are as follows:
 * `make test`
 * `make check`
 * `cmake --build . target test`
 * `cmake --build . target check`

### Translation

Note that files required for Quentier's translation are not built by default along with the app itself. In order to build
the translation files one needs to execute two additional commands:
```
make lupdate
make lrelease
```
The first one runs Qt's `lupdate` utility which extracts the strings requiring translation from the source code and updates
the `.ts` files containing both non-localized and localized text. The second command runs Qt's `lrelease` utility which
converts `.ts` files into installable `.qm` files. If during installation `.qm` files are present within the build directory,
they would be installed and used by the installed application

## Installation

The last step of building as written above is `make install` i.e. the installation of the built Quentier application.

The installation might involve creating the application bundle i.e. the standalone installation along with all the dependencies
which can be copied to another computer. On Windows that means copying the various dlls into the installation folder of the app,
on Mac it means creating an `.app` package. On Linux one can build Quentier as [AppImage](http://appimage.org) although
for this one additional dependency is required: [linuxdeployqt](https://github.com/probonopd/linuxdeployqt). Its location
can be hinted to `CMake` using `-DLINUXDEPLOYQT=<...path to linuxdeployqt executable>` switch.

The creation of an application bundle is enabled by default on Windows and OS X / macOS and disabled by default on Linux.
To manually change this option pass `-DCREATE_BUNDLE=ON` or `-DCREATE_BUNDLE=OFF` to `CMake` during building. Note that
for bundle creation `CMake` would need to find all Quentier's dependencies' own dependencies and that might require
additional hints to be given to `CMake`. The following additional libraries would be searched for:
 * qtkeychain (libquentier's dependency)
 * OpenSSL (libssl and libcrypto, libquentier's dependency; important note for OS X / macOS: system built-in libssl and libcrypto are not appropriate as they lack the encryption algorithm implementation used by libquentier; OpenSSL libraries from Homebrew or Macports should be used instead)
 * libxml2 (libquentier's dependency)
 * tidy-html5 (libquentier's dependency)
 * libhunspell (libquentier's dependency)
 * iconv (libxml2's dependency which in turn is libquentier's dependency)
 * zlib (libxml2's / iconv's dependency which in turn are libquentier's dependencies)
