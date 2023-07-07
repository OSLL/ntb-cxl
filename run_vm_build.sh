#!/bin/bash

CUR_PATH=$(pwd)
if [ $# -lt 1 ]; then
    echo -e "\e[1;33mBuild dir is not provided, using 'build_vm_image' dir\e[0m"
    BUILD_FOLDER_NAME=build_vm_image/
else
    BUILD_FOLDER_NAME="$1"
fi


BUILD_PATH=$CUR_PATH/$BUILD_FOLDER_NAME

mkdir $BUILD_FOLDER_NAME
docker build . --build-arg build_folder_name=$BUILD_FOLDER_NAME --build-arg user_id=$(id -u) -t yocto
docker run --network host -v ${BUILD_PATH}:/home/user/project/$BUILD_FOLDER_NAME yocto
