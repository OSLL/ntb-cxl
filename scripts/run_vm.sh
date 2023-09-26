#!/bin/bash

echo "Starting qemu vm..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

if [ "$QEMU_SHM_SIZE" ]; then
	VM1_OPTIONS="$VM1_OPTIONS \
		-object memory-backend-file,id=mem,size=${QEMU_SHM_SIZE}M,mem-path=/dev/shm/qemu,share=on \
		-machine memory-backend=mem -m ${QEMU_SHM_SIZE}m"
fi

runqemu qemux86-64 nographic slirp qemuparams="$COMMON_OPTIONS ${VM1_OPTIONS_OVERRIDE:-$VM1_OPTIONS}"

set +x

echo "Qemu vm exited."
