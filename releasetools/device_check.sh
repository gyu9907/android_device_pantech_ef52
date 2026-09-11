#!/sbin/sh

set -e
system_dir="${1:-/system}"

if grep -q "IM-A870S" /dev/block/mmcblk0p12 ; then
	mv "$system_dir"/vendor/etc/firmware_ef52s/* "$system_dir"/vendor/firmware/

elif grep -q "IM-A870K" /dev/block/mmcblk0p12 ; then
	mv "$system_dir"/vendor/etc/firmware_ef52k/* "$system_dir"/vendor/firmware/

elif grep -q "IM-A870L" /dev/block/mmcblk0p12 ; then
	mv "$system_dir"/vendor/etc/firmware_ef52l/* "$system_dir"/vendor/firmware/
else
    echo "Unable to identify EF52 firmware variant" >&2
    exit 1
fi

rm -rf "$system_dir"/vendor/etc/firmware_ef52s
rm -rf "$system_dir"/vendor/etc/firmware_ef52k
rm -rf "$system_dir"/vendor/etc/firmware_ef52l
rm -f "$system_dir"/bin/device_check.sh
