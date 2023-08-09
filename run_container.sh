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
docker build . --build-arg build_folder_name="$BUILD_FOLDER_NAME" --build-arg user_id="$(id -u)" -t yocto
docker run -it --rm -p 7000:7000 -p 7001:7001 -p 8000:8000 -p 8001:8001 \
	-v "$BUILD_PATH":/home/user/project/"$BUILD_FOLDER_NAME" \
	-v "$PWD"/qemu_src:/home/user/project/qemu_src \
	-v "$PWD"/yocto_files:/home/user/project/yocto_files \
	--env IVSHMEM_COMMON_OPTIONS="$IVSHMEM_COMMON_OPTIONS" \
	--env IVSHMEM_COMMON_OPTIONS_OVERRIDE="$IVSHMEM_COMMON_OPTIONS_OVERRIDE" \
	--env COMMON_OPTIONS="$COMMON_OPTIONS" \
	--env COMMON_OPTIONS_OVERRIDE="$COMMON_OPTIONS_OVERRIDE" \
	--env VM1_OPTIONS="$VM1_OPTIONS" \
	--env VM1_OPTIONS_OVERRIDE="$VM1_OPTIONS_OVERRIDE" \
	--env VM2_OPTIONS="$VM2_OPTIONS" \
	--env VM2_OPTIONS_OVERRIDE="$VM2_OPTIONS_OVERRIDE" \
	--env CMDLINE_COMMON="$CMDLINE_COMMON" \
	--env CMDLINE_COMMON_OVERRIDE="$COMMON_OPTIONS_OVERRIDE" \
	yocto "$CMD"
