#!/bin/sh
set -e

VENDOR=pantech
DEVICE=ef52
COMMON=msm8960-common

DEVICE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ANDROID_ROOT=$(CDPATH= cd -- "$DEVICE_DIR/../../.." && pwd)
VENDOR_ROOT=${VENDOR_ROOT:-"$ANDROID_ROOT/vendor/$VENDOR"}

# A local source is an unpacked, pre-installation ROM or product output root
# containing system/vendor (or vendor). Installation removes variant firmware.
if [ "$#" -gt 1 ]; then
    echo "Usage: $0 [adb|path-to-dump-root]" >&2
    exit 1
fi
SOURCE_ROOT=${1:-adb}
if [ "$SOURCE_ROOT" = adb ]; then
    adb get-state > /dev/null
else
    SOURCE_ROOT=$(CDPATH= cd -- "$SOURCE_ROOT" && pwd)
fi

# Keep the existing vendor tree intact until every blob and hash is verified.
mkdir -p "$VENDOR_ROOT"
STAGING_ROOT=$(mktemp -d "$VENDOR_ROOT/.extract-$DEVICE.XXXXXX")
trap 'rm -rf "$STAGING_ROOT"' EXIT
trap 'exit 1' HUP INT TERM

extract_manifest() {
    SCOPE=$1
    MANIFEST=$2
    BASE="$STAGING_ROOT/$SCOPE/proprietary"

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
        # Make variables describe installation paths, not literal ADB paths.
        case "$DESTINATION" in
            '$(TARGET_COPY_OUT_VENDOR)'/*)
                DESTINATION="vendor/${DESTINATION#*/}"
                ;;
            system/vendor/*)
                DESTINATION=${DESTINATION#system/}
                ;;
        esac
        mkdir -p "$BASE/$(dirname "$SOURCE")"
        if [ "$SOURCE_ROOT" = adb ]; then
            if ! adb pull "/$DESTINATION" "$BASE/$SOURCE"; then
                echo "Unable to extract $DESTINATION; use a complete pre-installation ROM dump for variant firmware." >&2
                exit 1
            fi
        else
            INPUT="$SOURCE_ROOT/$DESTINATION"
            if [ ! -f "$INPUT" ]; then
                case "$DESTINATION" in
                    vendor/*) INPUT="$SOURCE_ROOT/system/$DESTINATION" ;;
                esac
            fi
            if [ ! -f "$INPUT" ]; then
                echo "Missing $DESTINATION in $SOURCE_ROOT; use a complete pre-installation ROM dump." >&2
                exit 1
            fi
            cp -p "$INPUT" "$BASE/$SOURCE"
        fi

        if [ -n "$EXPECTED_HASH" ]; then
            echo "$EXPECTED_HASH  $BASE/$SOURCE" | sha256sum -c -
        fi
    done < "$MANIFEST"
}

extract_manifest "$DEVICE" "$DEVICE_DIR/proprietary-blobs.txt"
extract_manifest "$COMMON" "$DEVICE_DIR/../$COMMON/proprietary-blobs.txt"

VENDOR_ROOT="$STAGING_ROOT" "$DEVICE_DIR/setup-makefiles.sh"
for SCOPE in "$DEVICE" "$COMMON"; do
    mkdir -p "$VENDOR_ROOT/$SCOPE"
    rm -rf "$VENDOR_ROOT/$SCOPE/proprietary"
    mv "$STAGING_ROOT/$SCOPE/proprietary" "$VENDOR_ROOT/$SCOPE/proprietary"
    cp "$STAGING_ROOT/$SCOPE/"*.mk "$STAGING_ROOT/$SCOPE/Android.bp" "$VENDOR_ROOT/$SCOPE/"
done
