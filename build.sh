#!/bin/sh

FILE="$1"

UUID=`uuidgen`
NAME=`basename $0`

DIR=`dirname "$0"`
if [ $DIR = '.' ]; then
    DIR=$PWD
fi

BUILD_DIR="$DIR/build"
SRC_DIR="$DIR/src"

TMP_DIR="/tmp/$NAME-$UUID"

TMP_FILE="$TMP_DIR/ncLinuxApp.jar"

die () {
    echo $*
    exit 1
}

copy () {
    cp "$1" "$2" || die "Unable to copy '$1' to '$2'"
}

if [ ! -f "$FILE" ]; then
    die "Syntax $NAME <absolute path to ncLinuxApp.jar>"
fi

mkdir -p "$TMP_DIR"

cd $TMP_DIR

echo "Checking provided NC jar file..."
cp $FILE $TMP_FILE
md5sum --check $DIR/md5sums || die "Given ncLinuxApp.jar has wrong MD5"

echo "Cleaning previous build..."
rm -rf "$BUILD_DIR"

echo "Unpacking Network Connect client..."
unzip -qq $TMP_FILE || die "Unable to unpack ncLinuxLauncher"

echo "Copying files"
cp -r $DIR/src $BUILD_DIR

BUILD_SHARE_DIR=$BUILD_DIR/usr/share/jncclient

copy "$TMP_DIR/ncsvc"                 "$BUILD_SHARE_DIR"
copy "$TMP_DIR/ncdiag"                "$BUILD_SHARE_DIR"
copy "$TMP_DIR/getx509certificate.sh" "$BUILD_SHARE_DIR"

rm -rf "$TMP_DIR"

cp -r $DIR/debian $BUILD_DIR/DEBIAN

dpkg-deb -b "$BUILD_DIR" "$DIR/jncclient.deb"

