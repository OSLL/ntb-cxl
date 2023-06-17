#!/bin/bash

CUR_PATH=$(pwd)
BUILD_FOLDER_NAME=build_vm_image/


BUILD_PATH=$CUR_PATH/$BUILD_FOLDER_NAME

mkdir $BUILD_FOLDER_NAME
docker build . --build-arg build_folder_name=$BUILD_FOLDER_NAME -t yocto
docker run --network host -v ${BUILD_PATH}:/home/user/project/$BUILD_FOLDER_NAME yocto
