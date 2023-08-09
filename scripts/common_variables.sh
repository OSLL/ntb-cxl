#!/usr/bin/env bash

set -x
set -e

ROOT_PROJECT_PATH="$PWD"
YOCTO_VER_NAME="langdale"
LAYER_NAME="meta-ntb-cxl"

if [ $# -lt 1 ]; then
    echo -e "\e[1;33mBuild dir is not provided, using 'build_vm' dir\e[0m"
    YOCTO_WORK_DIR="$ROOT_PROJECT_PATH/build_vm"
else
    YOCTO_WORK_DIR="$1"
fi
if [ ! -d $YOCTO_WORK_DIR ]; then
    mkdir $YOCTO_WORK_DIR
fi

FULL_LAYER_NAME="$YOCTO_WORK_DIR"/poky/"$LAYER_NAME"

set +x
