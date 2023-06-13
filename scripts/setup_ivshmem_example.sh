#/bin/bash

set -x
set -e

LAYER_NAME="meta-ntb-cxl"
ROOT_PROJECT_PATH=$(pwd)

# Use path to poky dir with var POKY_PATH
cd $POKY_PATH/$LAYER_NAME
# Setup userspace programm
cp -r $ROOT_PROJECT_PATH/yocto_files/recipes-user .
cd recipes-user/ivshmem-user
mkdir -p files/src
cd files
wget https://raw.githubusercontent.com/Gavincrz/ivshmem_example/master/ivshmem_common.h
cd src
wget https://raw.githubusercontent.com/Gavincrz/ivshmem_example/master/userspace/userctl.c

# Setup ivshmem kernel module
cd $POKY_PATH/$LAYER_NAME/recipes-kernel
cp -r $ROOT_PROJECT_PATH/yocto_files/recipes-kernel/ivshmem-mod .
cd ivshmem-mod
mkdir -p files && cd files
wget https://raw.githubusercontent.com/Gavincrz/ivshmem_example/master/ivshmem_common.h
wget https://raw.githubusercontent.com/Gavincrz/ivshmem_example/master/kernel-module/ivshmem.c
wget https://raw.githubusercontent.com/Gavincrz/ivshmem_example/master/kernel-module/Makefile
# Replace include
sed -i 's/#include "..\/ivshmem_common.h"/#include "ivshmem_common.h"/g' ivshmem.c
