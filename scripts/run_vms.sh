#!/usr/bin/env bash

echo "Starting qemu virtual machines..."

set -x
set -e

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky

source oe-init-build-env build

#runqemu qemux86-64 nographic slirp

qemu=tmp/work/x86_64-linux/qemu-helper-native/1.0-r1/recipe-sysroot-native/usr/bin/qemu-system-x86_64

drive=tmp/deploy/images/qemux86-64/core-image-full-cmdline-qemux86-64-20230726131105.rootfs.ext4
kernel=tmp/deploy/images/qemux86-64/bzImage

"$qemu" \
	-usb -device usb-tablet -usb -device usb-kbd \
	-cpu IvyBridge -machine q35,i8042=off -smp 4 -m 256 \
	-serial telnet::8000,server,nowait -nographic -monitor null \
	-drive file="$drive",if=virtio,format=raw \
	-kernel "$kernel" \
	-append 'root=/dev/vda rw  ip=dhcp oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1' \
	&

cp -r tmp/deploy/images/qemux86-64 tmp/deploy/images/qemux86-64_2
drive2="${drive/qemux86-64\//qemux86-64_2/}"
kernel2="${kernel/qemux86-64\//qemux86-64_2/}"

"$qemu" \
	-usb -device usb-tablet -usb -device usb-kbd \
	-cpu IvyBridge -machine q35,i8042=off -smp 4 -m 256 \
	-serial telnet::8001,server,nowait -nographic -monitor null \
	-drive file="$drive2",if=virtio,format=raw \
	-kernel "$kernel2" \
	-append 'root=/dev/vda rw  ip=dhcp oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1' \
	&

wait

set +x

echo "All virtual machines exited."
