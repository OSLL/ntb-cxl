#/bin/bash

set -x

ROOT_PROJECT_PATH=$(pwd)
LAYER_NAME="meta-ntb-cxl"
if [ ! -d build_vm ]; then
    mkdir build_vm
fi
cd build_vm
if [ ! -d poky ]; then
    git clone git://git.yoctoproject.org/poky.git
fi
cd poky
if ! git checkout my-kirkstone; then
    git checkout -t origin/kirkstone  -b my-kirkstone
fi
git pull

source oe-init-build-env
if [ -z $(bitbake-layers show-layers | grep $LAYER_NAME) ]; then
    bitbake-layers create-layer ../$LAYER_NAME
fi
cp $ROOT_PROJECT_PATH/configs/yocto/local.conf ./conf/local.conf
cp $ROOT_PROJECT_PATH/yocto_files/bblayers.conf ./conf/bblayers.conf
cp -r $ROOT_PROJECT_PATH/yocto_files/recipes-kernel ../$LAYER_NAME/recipes-kernel

set +x
