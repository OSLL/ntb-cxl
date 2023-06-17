#!/bin/bash
CUR_PATH=$(pwd)
BUILD_FOLDER_NAME=build_vm_image/


BUILD_PATH=$CUR_PATH/$BUILD_FOLDER_NAME
docker run -it --network host -v ${BUILD_PATH}:/home/user/project/$BUILD_FOLDER_NAME -w /home/user/project/$BUILD_FOLDER_NAME/poky yocto bash -c "source oe-init-build-env && runqemu qemux86-64 nographic slirp"
