#!/bin/bash
set -xe

BUILD_TYPE=${1}
CLANG_FORMAT_BINARY=${2}
CLANG_TIDY_BINARY=${3}
QEVERCLOUD_DIR=${4}
LIBQUENTIER_DIR=${5}
QTKEYCHAIN_DIR=${6}

CDIR=$(pwd)

function error_exit {
    echo "$0: ***********error_exit***********"
    echo "***********" 1>&2
    echo "*********** Failed: $1" 1>&2
    echo "***********" 1>&2
    cd ${CDIR}
    exit 1
}

if [ ! -f bin/quentier/src/main.cpp ]; then
  echo "You seem to be in wrong directory. Script MUST be run from the project root directory."
  exit 1
fi

if [ -z "${BUILD_TYPE}" ]; then
    BUILD_TYPE=RelWithDebInfo
fi

if [ -z "${QEVERCLOUD_DIR}" ]; then
    QEVERCLOUD_DIR="$CDIR/../QEverCloud"
fi

if [ -z "${LIBQUENTIER_DIR}" ]; then
    LIBQUENTIER_DIR="$CDIR/../libquentier"
fi

if [ -z "${QTKEYCHAIN_DIR}" ]; then
	QTKEYCHAIN_DIR="$CDIR/../qtkeychain"
fi

echo "Building QEverCloud"
cd $QEVERCLOUD_DIR
mkdir -p builddir-${BUILD_TYPE}
cd builddir-${BUILD_TYPE}
cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX="$QEVERCLOUD_DIR/builddir-${BUILD_TYPE}/installdir" -DCMAKE_CXX_COMPILER_LAUNCHER=/usr/bin/ccache .. || error_exit "$0: cmake QEverCloud"
ninja || error_exit "$0: ninja QEverCloud"
ninja check || error_exit "$0: ninja check QEverCloud"
ninja install || error_exit "$0: ninja install QEverCloud"

echo "Building QtKeychain"
cd $QTKEYCHAIN_DIR
mkdir -p builddir-${BUILD_TYPE}
cd builddir-${BUILD_TYPE}
cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX="$QTKEYCHAIN_DIR/builddir-${BUILD_TYPE}/installdir" -DCMAKE_CXX_COMPILER_LAUNCHER=/usr/bin/ccache .. || error_exit "$0: cmake qtkeychain"
ninja || error_exit "$0: ninja qtkeychain"
ninja install || error_exit "$0: ninja install qtkeychain"

echo "Building libquentier"
cd $LIBQUENTIER_DIR
mkdir -p builddir-${BUILD_TYPE}
cd builddir-${BUILD_TYPE}
cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX="$LIBQUENTIER_DIR/builddir-${BUILD_TYPE}/installdir" -DQEverCloud-qt5_DIR="$QEVERCLOUD_DIR/builddir-${BUILD_TYPE}/installdir/lib/cmake/QEverCloud-qt5" -DQt5Keychain_DIR="$QTKEYCHAIN_DIR/builddir-${BUILD_TYPE}/installdir/lib/cmake/Qt5Keychain" -DCMAKE_CXX_COMPILER_LAUNCHER=/usr/bin/ccache -DCLANG_FORMAT_BINARY=$CLANG_FORMAT_BINARY -DCLANG_TIDY_BINARY=$CLANG_TIDY_BINARY .. || error_exit "$0: cmake libquentier"
ninja || error_exit "$0: ninja libquentier"
# Might as well run ninja check but it seems to turn each single gtest instance into a separate launch and it takes ages
xvfb-run ./test_libquentier -platform minimal || error_exit "$0: test_libquentier"
xvfb-run ./src/enml/tests/libqt5quentier_enml_unit_tests -platform minimal || error_exit "$0: libqt5quentier_enml_unit_tests"
xvfb-run ./src/local_storage/sql/tests/libqt5quentier_local_storage_sql_unit_tests -platform minimal || error_exit "$0: libqt5quentier_local_storage_sql_unit_tests"
xvfb-run ./src/synchronization/tests/libqt5quentier_synchronization_unit_tests -platform minimal || error_exit "$0: libqt5quentier_synchronization_unit_tests"
xvfb-run ./src/threading/tests/libqt5quentier_threading_unit_tests -platform minimal || error_exit "$0: libqt5quentier_threading_unit_tests"
xvfb-run ./src/utility/tests/libqt5quentier_utility_unit_tests -platform minimal || error_exit "$0: libqt5quentier_utility_unit_tests"
xvfb-run ./tests/synchronization/libqt5quentier_synchronization_integrational_tests -platform minimal || error_exit "$0: libqt5quentier_synchronization_integrational_tests"
ninja lupdate || error_exit "$0: ninja lupdate libquentier"
ninja lrelease || error_exit "$0: ninja lrelease libquentier"
ninja install || error_exit "$0: ninja install libquentier"

echo "Building quentier"
cd $CDIR
mkdir -p builddir-${BUILD_TYPE}
cd builddir-${BUILD_TYPE}
cmake -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=/usr -DQEverCloud-qt5_DIR="$QEVERCLOUD_DIR/builddir-${BUILD_TYPE}/installdir/lib/cmake/QEverCloud-qt5" -DLibquentier-qt5_DIR="$LIBQUENTIER_DIR/builddir-${BUILD_TYPE}/installdir/lib/cmake/Libquentier-qt5" -DCMAKE_CXX_COMPILER_LAUNCHER=/usr/bin/ccache -DCLANG_FORMAT_BINARY=$CLANG_FORMAT_BINARY -DCLANG_TIDY_BINARY=$CLANG_TIDY_BINARY .. || error_exit "$0: cmake quentier"
ninja || error_exit "$0: ninja quentier"
xvfb-run ./lib/model/tests/quentier_model_tests -platform minimal || error_exit "$0: ninja check quentier"
ninja lupdate || error_exit "$0: ninja lupdate quentier"
ninja lrelease || error_exit "$0: ninja lrelease quentier"
DESTDIR=$CDIR/appdir ninja install || error_exit "$0: ninja install quentier"

DESKTOP_FILE=$CDIR/appdir/usr/share/applications/org.quentier.Quentier.desktop
if [ ! -f "$DESKTOP_FILE" ]; then
    echo "$DESKTOP_FILE not found!"
    exit 1
fi

echo "Deploying dependencies"
EXTRA_QT_PLUGINS=svg LD_LIBRARY_PATH="$QEVERCLOUD_DIR/builddir-${BUILD_TYPE}/installdir/lib:$LIBQUENTIER_DIR/builddir-${BUILD_TYPE}/installdir/lib" linuxdeploy --desktop-file="$DESKTOP_FILE" --appdir="$CDIR/appdir" --plugin qt

echo "Removing deployed offending libs"
rm -rf $CDIR/appdir/usr/lib/libnss3.so
rm -rf $CDIR/appdir/usr/lib/libnssutil3.so

echo "Copying libquentier translations"
mkdir -p $CDIR/appdir/usr/share/libquentier/translations
cp -r $LIBQUENTIER_DIR/builddir-${BUILD_TYPE}/installdir/share/libquentier/translations/*.qm $CDIR/appdir/usr/share/libquentier/translations/

echo "Creating AppImage"
appimagetool -n $CDIR/appdir
