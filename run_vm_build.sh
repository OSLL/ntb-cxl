PATH=$(pwd)
BUILD_FOLDER_NAME=build_vm_image/
BUILD_PATH=$PATH/$BUILD_FOLDER_NAME

docker build . --build-arg build_folder_name=$BUILD_FOLDER_NAME -t yocto_build

docker run --network host yocto_build -v $BUILD_PATH:/build_vm_image/