#!/usr/bin/env bash

echo "Starting qemu virtual machines..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

#runqemu qemux86-64 nographic slirp

qemu=tmp/work/x86_64-linux/qemu-system-native/7.1.0-r0/qemu-7.1.0/qemu-system-x86_64
ivshmem_server=tmp/work/x86_64-linux/qemu-system-native/7.1.0-r0/qemu-7.1.0/contrib/ivshmem-server/ivshmem-server

drive=tmp/deploy/images/qemux86-64/core-image-full-cmdline-qemux86-64.ext4
kernel=tmp/deploy/images/qemux86-64/bzImage

cp -r tmp/deploy/images/qemux86-64 tmp/deploy/images/qemux86-64_2
drive2="${drive/qemux86-64\//qemux86-64_2/}"
kernel2="${kernel/qemux86-64\//qemux86-64_2/}"

IVSHMEM_COMMON_OPTIONS_DEFAULT="-device ivshmem-doorbell,vectors=2,chardev=ivshmem \
	-chardev socket,path=/tmp/ivshmem_socket,id=ivshmem"

if [ "$IVSHMEM_COMMON_OPTIONS_OVERRIDE" ]; then
	IVSHMEM_COMMON_OPTIONS="$IVSHMEM_COMMON_OPTIONS_OVERRIDE"
else
	IVSHMEM_COMMON_OPTIONS="$IVSHMEM_COMMON_OPTIONS_DEFAULT $IVSHMEM_COMMON_OPTIONS"
fi

COMMON_OPTIONS_DEFAULT="-usb -device usb-tablet -usb -device usb-kbd \
	-cpu IvyBridge -machine q35,i8042=off -smp 4 -m 256 \
	-nographic -monitor null \
	-append 'root=/dev/vda rw  ip=dhcp oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1'"
VM1_OPTIONS_DEFAULT="-serial telnet::8000,server,nowait -append ivshmem_master"
VM2_OPTIONS_DEFAULT="-serial telnet::8001,server,nowait"

if [ "$COMMON_OPTIONS_OVERRIDE" ]; then
	COMMON_OPTIONS="$COMMON_OPTIONS_OVERRIDE $IVSHMEM_COMMON_OPTIONS"
else
	COMMON_OPTIONS="$COMMON_OPTIONS_DEFAULT $COMMON_OPTIONS $IVSHMEM_COMMON_OPTIONS"
fi

if [ "$VM1_OPTIONS_OVERRIDE" ]; then
	VM1_OPTIONS="$VM1_OPTIONS_OVERRIDE"
else
	VM1_OPTIONS="$VM1_OPTIONS_DEFAULT $VM1_OPTIONS"
fi

if [ "$VM2_OPTIONS_OVERRIDE" ]; then
	VM2_OPTIONS="$VM2_OPTIONS_OVERRIDE"
else
	VM2_OPTIONS="$VM2_OPTIONS_DEFAULT $VM2_OPTIONS"
fi

"$ivshmem_server" -m . -l 1M -n 2 -F &

"$qemu" \
	-drive file="$drive",if=virtio,format=raw \
	-kernel "$kernel" \
	$COMMON_OPTIONS \
	$VM1_OPTIONS &

"$qemu" \
	-drive file="$drive2",if=virtio,format=raw \
	-kernel "$kernel2" \
	$COMMON_OPTIONS \
	$VM2_OPTIONS &

wait

set +x

echo "All virtual machines exited."
