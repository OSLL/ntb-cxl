#!/usr/bin/env bash

set -e

ROOT_PROJECT_PATH="$PWD"
YOCTO_VER_NAME="langdale"
LAYER_NAME="meta-ntb-cxl"

YOCTO_WORK_DIR=$BUILD_PATH
if [ ! -d $YOCTO_WORK_DIR ]; then
    mkdir $YOCTO_WORK_DIR
fi

FULL_LAYER_NAME="$YOCTO_WORK_DIR"/poky/"$LAYER_NAME"
