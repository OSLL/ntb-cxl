#/bin/bash

set -x
set -e

ROOT_PROJECT_PATH=$(pwd)
YOCTO_VER_NAME="langdale"
LAYER_NAME="meta-ntb-cxl"
if [ $# -lt 1 ]; then
    echo -e "\e[1;33mBuild dir is not provided, using 'build_vm' dir\e[0m"
    YOCTO_WORK_DIR="build_vm"
else
    YOCTO_WORK_DIR="$1"
fi
if [ ! -d $YOCTO_WORK_DIR ]; then
    mkdir $YOCTO_WORK_DIR
fi
cd $YOCTO_WORK_DIR
if [ ! -d poky ]; then
    git clone git://git.yoctoproject.org/poky.git
fi
cd poky
if ! git checkout my-$YOCTO_VER_NAME; then
    git checkout -t origin/$YOCTO_VER_NAME  -b my-$YOCTO_VER_NAME
fi
git pull

FULL_LAYER_NAME="$(pwd)/$LAYER_NAME"
source oe-init-build-env build
if [ -z "$(bitbake-layers show-layers | grep $LAYER_NAME)" ]; then
    bitbake-layers create-layer ../$LAYER_NAME
    echo -e "BBLAYERS += \"$FULL_LAYER_NAME\"" >> ./conf/bblayers.conf
fi
cp $ROOT_PROJECT_PATH/yocto_files/configs/local.conf ./conf/local.conf
cp -rT $ROOT_PROJECT_PATH/yocto_files/recipes-kernel ../$LAYER_NAME/recipes-kernel
cp -rT $ROOT_PROJECT_PATH/yocto_files/recipes-devtools ../$LAYER_NAME/recipes-devtools
cp -rT $ROOT_PROJECT_PATH/yocto_files/recipes-connectivity ../$LAYER_NAME/recipes-connectivity

bitbake -c kernel_configme -f virtual/kernel && \
bitbake -c compile -f virtual/kernel && \
bitbake -c deploy virtual/kernel && \
bitbake core-image-full-cmdline && \
bitbake qemu-system-native

set +x
