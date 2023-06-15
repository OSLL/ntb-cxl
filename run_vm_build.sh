#!/bin/bash

PATH=$(pwd)
BUILD_FOLDER_NAME=build_vm_image/


BUILD_PATH=$PATH/$BUILD_FOLDER_NAME

/usr/bin/docker build . --build-arg build_folder_name=$BUILD_FOLDER_NAME -t yocto_build
/usr/bin/docker run --network host -v ${BUILD_PATH}:/project/$BUILD_FOLDER_NAME yocto_build
