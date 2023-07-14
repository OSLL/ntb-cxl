#!/usr/bin/env bash

echo "Starting prepare_yocto..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd $YOCTO_WORK_DIR
if [ ! -d poky ]; then
    git clone git://git.yoctoproject.org/poky.git
fi
cd poky
if ! git checkout my-$YOCTO_VER_NAME; then
    git checkout -t origin/$YOCTO_VER_NAME  -b my-$YOCTO_VER_NAME
fi
git pull

source oe-init-build-env build
if [ -z "$(bitbake-layers show-layers | grep $LAYER_NAME)" ]; then
    bitbake-layers create-layer ../$LAYER_NAME
    echo -e "BBLAYERS += \"$FULL_LAYER_NAME\"" >> ./conf/bblayers.conf
fi
cp $ROOT_PROJECT_PATH/yocto_files/configs/local.conf ./conf/local.conf
cp -r $ROOT_PROJECT_PATH/yocto_files/recipes-kernel ../$LAYER_NAME/recipes-kernel
cp -r $ROOT_PROJECT_PATH/yocto_files/recipes-devtools ../$LAYER_NAME/recipes-devtools

set +x

echo "Finished prepare_yocto"
