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

cp $ROOT_PROJECT_PATH/yocto_files/configs/local.conf ./conf/local.conf
rm -r ../$LAYER_NAME || true # cleanup
cp -r $ROOT_PROJECT_PATH/yocto_files/$LAYER_NAME ../$LAYER_NAME

# Add layer to bitbake if it has not already been added
if [ -z "$(bitbake-layers show-layers | grep $LAYER_NAME)" ]; then
    bitbake-layers add-layer ../$LAYER_NAME
fi

set +x

echo "Finished prepare_yocto"
