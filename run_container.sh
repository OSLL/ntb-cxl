#!/bin/bash

if [ "$1" ]; then
    CMD="$1"

    if ! [ "$2" ]; then
        echo -e "\e[1;33mBuild dir is not provided, using 'build_vm_image' dir\e[0m"
        BUILD_FOLDER_NAME=build_vm_image/
    else
        BUILD_FOLDER_NAME="$2"
    fi
else
    echo -e "\e[1;33mCommand is not provided, assuming 'build'\e[0m"
    CMD=build

    echo -e "\e[1;33mBuild dir is not provided, using 'build_vm_image' dir\e[0m"
    BUILD_FOLDER_NAME=build_vm_image/
fi

BUILD_PATH="$PWD"/"$BUILD_FOLDER_NAME"

mkdir -p "$BUILD_FOLDER_NAME"
docker build . --build-arg build_folder_name="$BUILD_FOLDER_NAME" --build-arg user_id="$(id -u)" -t yocto
docker run --rm --network host -v "$BUILD_PATH":/home/user/project/"$BUILD_FOLDER_NAME" yocto "$CMD"
