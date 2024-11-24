#!/bin/bash

ROOT_DIR=$(pwd)
BUILD_DIR="${ROOT_DIR}/build-pack"
APP_DIR="${BUILD_DIR}/AppDir"
LIB_DIR="${BUILD_DIR}/AppDir/usr/lib"
RESOURCE_DIR="${ROOT_DIR}/psvrplayer/resources"
SYSLIB_DIR="/usr/lib/x86_64-linux-gnu"
APPIMAGE=/usr/local/bin/appimagetool-x86_64.AppImage

pushd ${BUILD_DIR}
rm -rfd AppDir

echo Packing psvrplayer ...
mkdir AppDir
mkdir AppDir/usr
mkdir AppDir/usr/lib

pushd AppDir
ln -s psvrplayer AppRun
popd
cp output/psvrplayer "${APP_DIR}/"
cp "${RESOURCE_DIR}/psvrplayer.desktop" AppDir/
cp "${RESOURCE_DIR}/psvrplayer_256.png" AppDir/psvrplayer.png

cp "${SYSLIB_DIR}/libglfw.so.3" "${LIB_DIR}/"
# cp "${SYSLIB_DIR}/libvlc.so.5" "${LIB_DIR}/"
# cp "${SYSLIB_DIR}/libvlccore.so.9" "${LIB_DIR}/"


cp "${SYSLIB_DIR}/libhidapi-libusb.so.0" "${LIB_DIR}/"


ARCH=x86_64 ${APPIMAGE} AppDir

popd






