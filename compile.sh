#!/bin/bash

BUILD_DIR=build-pack
APPIMAGE=/usr/local/bin/appimagetool-x86_64.AppImage

rm -frd ${BUILD_DIR}
mkdir ${BUILD_DIR}
pushd ${BUILD_DIR}

echo Compiling psvrplayer ...
cmake -D EXECUTABLE_OUTPUT_PATH="$(pwd)/output" ../
cmake --build .

echo Packing psvrplayer ...
mkdir AppDir
pushd AppDir
ln -s psvrplayer AppRun
popd
cp output/psvrplayer AppDir/

ARCH=x86_64 ${APPIMAGE} AppDir

popd





