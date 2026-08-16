#!/bin/sh
set -e

VENDOR=pantech
DEVICE=ef52
COMMON=msm8960-common

DEVICE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ANDROID_ROOT=$(CDPATH= cd -- "$DEVICE_DIR/../../.." && pwd)

extract_manifest() {
    SCOPE=$1
    MANIFEST=$2
    BASE="$ANDROID_ROOT/vendor/$VENDOR/$SCOPE/proprietary"

    rm -rf "$BASE"
    mkdir -p "$BASE"

    while IFS= read -r ENTRY || [ -n "$ENTRY" ]; do
        case "$ENTRY" in
            ""|"#"*) continue ;;
        esac

        EXPECTED_HASH=
        case "$ENTRY" in
            *"|"*)
                EXPECTED_HASH=${ENTRY##*|}
                ENTRY=${ENTRY%%|*}
                ;;
        esac

        SOURCE=${ENTRY%%:*}
        DESTINATION=${ENTRY#*:}
        mkdir -p "$BASE/$(dirname "$SOURCE")"
        adb pull "/$DESTINATION" "$BASE/$SOURCE"

        if [ -n "$EXPECTED_HASH" ]; then
            echo "$EXPECTED_HASH  $BASE/$SOURCE" | sha256sum -c -
        fi
    done < "$MANIFEST"
}

extract_manifest "$DEVICE" "$DEVICE_DIR/proprietary-blobs.txt"
extract_manifest "$COMMON" "$DEVICE_DIR/../$COMMON/proprietary-blobs.txt"

"$DEVICE_DIR/setup-makefiles.sh"
