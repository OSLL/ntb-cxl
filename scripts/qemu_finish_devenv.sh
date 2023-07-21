#!/usr/bin/env bash

echo "Applying changes..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

devtool finish qemu-system-native ../meta-ntb-cxl

DESTDIR="$ROOT_PROJECT_PATH"/yocto_files/recipes-devtools/qemu
mkdir -p "$DESTDIR"
cd ../meta-ntb-cxl/recipes-devtools/qemu

# obsolete patches will remain here, remove them
rm -r "$DESTDIR"/qemu-system-native || true

cp -r qemu-system-native "$DESTDIR"
cp qemu-system-native_%.bbappend "$DESTDIR"

set +x

echo "Finished applying changes"
