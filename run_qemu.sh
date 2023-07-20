#!/bin/bash
CUR_PATH=$(pwd)
CONTAINER_NAME="yocto_demo_pingpong"
if [ $# -lt 1 ]; then
    echo "Provide command for execution"
    exit 1
fi

if [ $# -lt 2 ]; then
    echo -e "\e[1;33mBuild dir is not provided, using 'build_vm_image' dir\e[0m"
    BUILD_FOLDER_NAME=build_vm_image/
else
    BUILD_FOLDER_NAME="$2"
fi


BUILD_PATH=$CUR_PATH/$BUILD_FOLDER_NAME
if [ -z "$(docker container ps | grep $CONTAINER_NAME)" ]; then 
    DOCKER_PREFIX="docker run --rm -it -v ${BUILD_PATH}:/home/user/project/$BUILD_FOLDER_NAME -w /home/user/project/ --name $CONTAINER_NAME -w /home/user/project/$BUILD_FOLDER_NAME/poky yocto"
else
    DOCKER_PREFIX="docker exec -it $CONTAINER_NAME"
fi

$DOCKER_PREFIX /bin/bash -c "source oe-init-build-env && export LD_LIBRARY_PATH='/usr/lib' &&\$PROJECT_PATH/scripts/run_command.sh $1"
# docker run -it --network host -v ${BUILD_PATH}:/home/user/project/$BUILD_FOLDER_NAME -w /home/user/project/$BUILD_FOLDER_NAME/poky yocto bash -c "source oe-init-build-env && runqemu qemux86-64 nographic slirp"
