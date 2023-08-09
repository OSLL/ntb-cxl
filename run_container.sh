#!/bin/bash

if [ "$1" ]; then
    CMD="$1"
else
    echo -e "\e[1;33mCommand is not provided, assuming 'build'\e[0m"
    CMD=build
fi

if [ "$2" ]; then
    BUILD_FOLDER_NAME="$2"
else
    echo -e "\e[1;33mBuild dir is not provided, using 'build_vm_image' dir\e[0m"
    BUILD_FOLDER_NAME=build_vm_image/
fi

BUILD_PATH="$PWD"/"$BUILD_FOLDER_NAME"

mkdir -p "$BUILD_FOLDER_NAME"
docker build . --build-arg user_id="$(id -u)" -t yocto
docker run --rm --network host \
	-v "$BUILD_PATH":/home/user/project/build_dir \
	-v "$PWD"/qemu_src:/home/user/project/qemu_src \
	-v "$PWD"/yocto_files:/home/user/project/yocto_files \
	yocto "$CMD"
