#!/usr/bin/env bash

echo "Applying changes..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

RECIPE_NAME="qemu-system-native"
DIR_WITH_PATCHES="qemu"

devtool finish $RECIPE_NAME $LAYER_NAME

DESTDIR="$ROOT_PROJECT_PATH"/yocto_files/$LAYER_NAME/recipes-devtools/qemu
mkdir -p "$DESTDIR"
cd ../$LAYER_NAME/recipes-devtools/qemu

# obsolete patches will remain here, remove them
# rm -r "$DESTDIR"/$RECIPE_NAME || true

cp -Tru $DIR_WITH_PATCHES "$DESTDIR/$DIR_WITH_PATCHES"
cp $RECIPE_NAME\_8.0.2.bbappend "$DESTDIR"

set +x

echo "Finished applying changes"
