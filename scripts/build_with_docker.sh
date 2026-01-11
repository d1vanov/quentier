#!/bin/bash
QEVERCLOUD_BRANCH=${1}
LIBQUENTIER_BRANCH=${2}
QUENTIER_BRANCH=${3}
QTKEYCHAIN_BRANCH=${4}
PROJECTDIR=$(pwd)
set -xe

DOCKERTAG=quentier/focal
DOCKERFILE=./scripts/dockerfiles/Dockerfile.ubuntu_focal

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

if [ -z "${QEVERCLOUD_BRANCH}" ]; then
    QEVERCLOUD_BRANCH=master
fi

if [ -z "${LIBQUENTIER_BRANCH}" ]; then
    LIBQUENTIER_BRANCH=master
fi

if [ -z "${QUENTIER_BRANCH}" ]; then
    QUENTIER_BRANCH=master
fi

if [ -z "${QTKEYCHAIN_BRANCH}" ]; then
	QTKEYCHAIN_BRANCH=0.14.3
fi

cd $PROJECTDIR
# create "builder" image
docker build -t ${DOCKERTAG} -f ${DOCKERFILE} ./scripts/dockerfiles

# stop after creating the image (e.g. you want to do the build manually)
if [ ! -d appdir ] ; then
  mkdir appdir || error_exit "mkdir appdir"
fi

# delete appdir contents
rm -rf appdir/* || error_exit "rm appdir"

BUILD_TYPE=RelWithDebInfo

if [ ! -d docker-build-QEverCloud-${BUILD_TYPE} ]; then
  mkdir docker-build-QEverCloud-${BUILD_TYPE}
fi

if [ ! -d docker-build-libquentier-${BUILD_TYPE} ]; then
  mkdir docker-build-libquentier-${BUILD_TYPE}
fi

if [ ! -d docker-build-quentier-${BUILD_TYPE} ]; then
  mkdir docker-build-quentier-${BUILD_TYPE}
fi

if [ ! -d docker-build-ccache ]; then
  mkdir docker-build-ccache
fi

time docker run \
   --rm \
   -v $PROJECTDIR/docker-build-ccache:/opt/ccache \
   -e CCACHE_DIR=/opt/ccache \
   -v $PROJECTDIR/docker-build-QEverCloud-${BUILD_TYPE}:/opt/QEverCloud/builddir-${BUILD_TYPE} \
   -v $PROJECTDIR/docker-build-qtkeychain-${BUILD_TYPE}:/opt/qtkeychain/builddir-${BUILD_TYPE} \
   -v $PROJECTDIR/docker-build-libquentier-${BUILD_TYPE}:/opt/libquentier/builddir-${BUILD_TYPE} \
   -v $PROJECTDIR/docker-build-quentier-${BUILD_TYPE}:/opt/quentier/builddir-${BUILD_TYPE} \
   -v $PROJECTDIR/appdir:/opt/quentier/appdir \
   -it ${DOCKERTAG} \
      /bin/bash -c "cd /opt/QEverCloud && git fetch && git checkout $QEVERCLOUD_BRANCH && git reset --hard origin/$QEVERCLOUD_BRANCH && cd /opt/libquentier && git fetch && git checkout $LIBQUENTIER_BRANCH && git reset --hard origin/$LIBQUENTIER_BRANCH && cd /opt/quentier && git fetch && git checkout $QUENTIER_BRANCH && git reset --hard origin/$QUENTIER_BRANCH && ./scripts/build_with_cmake.sh ${BUILD_TYPE} /usr/bin/clang-format /usr/bin/clang-tidy && mv builddir-${BUILD_TYPE}/*.AppImage appdir && chmod -R a+rwx appdir/*.AppImage"

ls appdir/*.AppImage
echo "If all got well then AppImage file should be in appdir folder"

cd $PROJECTDIR
