#!/bin/sh

VENDOR=pantech
DEVICE=ef52
QSEECOMD_SHA256=5cbf7b7a9a9a3b5ba45bc9d7978f410f3d1ce20fca19f821e1b8b7b44ab80d98

BASE=../../../vendor/$VENDOR/$DEVICE/proprietary
rm -rf $BASE/*

for FILE in `cat proprietary-blobs.txt | grep -v ^# | grep -v ^$ `; do
    DIR=`dirname $FILE`
    if [ ! -d $BASE/$DIR ]; then
        mkdir -p $BASE/$DIR
    fi
    adb pull /system/$FILE $BASE/$FILE
    if [ "$FILE" = "bin/qseecomd" ]; then
        echo "$QSEECOMD_SHA256  $BASE/$FILE" | sha256sum -c - || exit 1
    fi
done
./copy_from_target.sh
./setup-makefiles.sh
