#!/bin/bash
BUILD_ROOT=/root/build

SPANK_REPO=https://github.com/noname22/spank.git
SPANK=$BUILD_ROOT/spank/spank

ARTIFACTS="NetCleanFrameServer.exe NetCleanFrameServer.exe.debug"
FRAMESERVERSDK_ARTIFACTS="frameserver_sdk/videosdk.dll"

set -e

# set up build directory
mkdir -p $BUILD_ROOT

# make a copy of the source tree and build there
# because strip fails in docker on windows (virtualbox shared folders)
# https://www.virtualbox.org/ticket/8463

cd $BUILD_ROOT
cp -r source source-copy
cd source-copy

# clone and build spank
git clone $SPANK_REPO $BUILD_ROOT/spank
cd $BUILD_ROOT/spank
./build.sh
cp spank /usr/bin

# build frameserver_sdk
cd $BUILD_ROOT/source-copy/frameserver_sdk
spank build

# build release
cd $BUILD_ROOT/source-copy
$SPANK --jobs 4 rebuild release

# copy back the artifacts to the mounted directory
cp $ARTIFACTS ../source
cp $FRAMESERVERSDK_ARTIFACTS ../source/frameserver_sdk
