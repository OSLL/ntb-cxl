#!/bin/bash

RUN_ARGS=""

for ARG in "$@"; do
	case $ARG in
		--host-build-dir=*)
			BUILD_FOLDER_NAME=${ARG#*=}
			;;
		*)
			RUN_ARGS+="$ARG "
			;;
	esac
done

if [ -z "$BUILD_FOLDER_NAME" ]; then
    echo -e "\e[1;33mHost build dir is not provided, using 'build_vm_image' dir\e[0m"
    BUILD_FOLDER_NAME=build_vm_image/
fi

BUILD_PATH="$PWD"/"$BUILD_FOLDER_NAME"

mkdir -p "$BUILD_FOLDER_NAME"
docker build . --build-arg user_id="$(id -u)" -t yocto
docker run -it --rm -p 7000:7000 -p 7001:7001 -p 8000:8000 -p 8001:8001 \
	-v "$BUILD_PATH":/home/user/project/build_dir \
	-v "$PWD"/qemu_src:/home/user/project/qemu_src \
	-v "$PWD"/yocto_files:/home/user/project/yocto_files \
	yocto $RUN_ARGS
