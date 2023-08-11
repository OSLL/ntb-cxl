#!/usr/bin/env bash

echo "Starting qemu virtual machines..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky/build

qemu="$(find ./tmp/work/x86_64-linux/qemu-system-native -type f -name qemu-system-x86_64 | head -n1)"
ivshmem_server="$(find ./tmp/work/x86_64-linux/qemu-system-native -type f -name ivshmem-server | head -n1)"

vm1_dir=guest_1
vm2_dir=guest_2

drive=$vm1_dir/core-image-full-cmdline-qemux86-64.ext4
kernel=$vm1_dir/bzImage
drive2=$vm2_dir/core-image-full-cmdline-qemux86-64.ext4
kernel2=$vm2_dir/bzImage

IVSHMEM_COMMON_OPTIONS_DEFAULT="-device ivshmem-doorbell,vectors=2,chardev=ivshmem \
	-chardev socket,path=/tmp/ivshmem_socket,id=ivshmem"

if [ "$IVSHMEM_COMMON_OPTIONS_OVERRIDE" ]; then
	IVSHMEM_COMMON_OPTIONS="$IVSHMEM_COMMON_OPTIONS_OVERRIDE"
else
	IVSHMEM_COMMON_OPTIONS="$IVSHMEM_COMMON_OPTIONS_DEFAULT $IVSHMEM_COMMON_OPTIONS"
fi

CMDLINE_COMMON_DEFAULT="root=/dev/vda rw ip=dhcp oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1"

if [ "$CMDLINE_COMMON_OVERRIDE" ]; then
	CMDLINE_COMMON="$CMDLINE_COMMON_OVERRIDE"
else
	CMDLINE_COMMON="$CMDLINE_COMMON_DEFAULT $CMDLINE_COMMON"
fi

COMMON_OPTIONS_DEFAULT="-usb -device usb-tablet -usb -device usb-kbd \
	-cpu IvyBridge -machine q35,i8042=off -smp 4 -m 256 \
	-nographic -monitor null"

VM1_OPTIONS_DEFAULT="-serial telnet::8000,server,nowait -net nic -net user,hostfwd=tcp::7000-:22"
VM2_OPTIONS_DEFAULT="-serial telnet::8001,server,nowait -net nic -net user,hostfwd=tcp::7001-:22"

if [ "$COMMON_OPTIONS_OVERRIDE" ]; then
	COMMON_OPTIONS="$COMMON_OPTIONS_OVERRIDE"
else
	COMMON_OPTIONS="$COMMON_OPTIONS_DEFAULT $COMMON_OPTIONS"
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
	-append "$CMDLINE_COMMON ivshmem_master" \
	$COMMON_OPTIONS \
	$IVSHMEM_COMMON_OPTIONS \
	$VM1_OPTIONS &

"$qemu" \
	-drive file="$drive2",if=virtio,format=raw \
	-kernel "$kernel2" \
	-append "$CMDLINE_COMMON" \
	$COMMON_OPTIONS \
	$IVSHMEM_COMMON_OPTIONS \
	$VM2_OPTIONS &

wait

set +x

echo "All virtual machines exited."
