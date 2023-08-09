#!/usr/bin/env bash

echo "Starting build_yocto..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

bitbake -c kernel_configme -f virtual/kernel && \
bitbake -c compile -f virtual/kernel && \
bitbake -c deploy virtual/kernel && \
bitbake qemu-system-native && \
bitbake core-image-full-cmdline

set +x

echo "finished build_yocto"
