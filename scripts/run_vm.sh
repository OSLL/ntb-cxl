#!/bin/bash

echo "Starting qemu vm..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

runqemu qemux86-64 nographic slirp qemuparams="$COMMON_OPTIONS ${VM1_OPTIONS_OVERRIDE:-$VM1_OPTIONS}"

set +x

echo "Qemu vm exited."
