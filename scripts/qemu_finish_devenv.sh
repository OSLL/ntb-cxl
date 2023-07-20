#!/usr/bin/env bash

echo "Applying changes..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

devtool finish qemu-native ../meta-ntb-cxl

DESTDIR="$ROOT_PROJECT_PATH"/yocto_files/recipes-devtools/qemu
mkdir -p "$DESTDIR"
cd ../meta-ntb-cxl/recipes-devtools/qemu

# obsolete patches will remain here, remove them
rm -r "$DESTDIR"/qemu-native || true

cp -r qemu-native "$DESTDIR"
rm "$DESTDIR"/qemu-native/cross.patch || true # why is it there anyway?
cp qemu-native_%.bbappend "$DESTDIR"

set +x

echo "Finished applying changes"
