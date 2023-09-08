#!/bin/bash

RUN_ARGS=()
BUILD_NO_CACHE=""

for ARG in "$@"; do
    case $ARG in
        --host-build-dir=*)
            BUILD_FOLDER_NAME=${ARG#*=}
            ;;
        --docker-no-cache*)
            BUILD_NO_CACHE="--no-cache"
            ;;
        *)
            RUN_ARGS+=("$ARG")
            ;;
    esac
done

if [ -z "$BUILD_FOLDER_NAME" ]; then
    echo -e "\e[1;33mHost build dir is not provided, using 'build_vm_image' dir\e[0m"
    BUILD_FOLDER_NAME=build_vm_image/
fi

BUILD_PATH="$PWD"/"$BUILD_FOLDER_NAME"

mkdir -p "$BUILD_FOLDER_NAME"
docker build . --build-arg user_id="$(id -u)" -t yocto $BUILD_NO_CACHE
docker run -it --rm -p 7001:7001 -p 7002:7002 -p 8001:8001 -p 8002:8002 \
    -v "$BUILD_PATH":/home/user/project/build_dir \
    -v "$PWD"/qemu_src:/home/user/project/qemu_src \
    -v "$PWD"/yocto_files:/home/user/project/yocto_files \
    yocto "${RUN_ARGS[@]}"
