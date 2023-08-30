#!/usr/bin/env bash

echo "Starting build_yocto..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

if [[ $BUILD_VAL == "all" || $BUILD_VAL == "image" ]]; then
    bitbake -c kernel_configme -f virtual/kernel
    bitbake -c compile -f virtual/kernel
    bitbake -c deploy virtual/kernel
    bitbake core-image-full-cmdline

    # Create two image copies for VMs
    cp -ruT tmp/deploy/images/qemux86-64/ ./guest_1
    cp -ruT tmp/deploy/images/qemux86-64/ ./guest_2
fi

if [[ $BUILD_VAL == "all" || $BUILD_VAL == "qemu" ]]; then
    bitbake -c install qemu-system-native
    bitbake qemu-helper-native
fi

set +x

echo "finished build_yocto"
