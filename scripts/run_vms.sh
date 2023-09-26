#!/usr/bin/env bash

echo "Starting qemu virtual machines..."

set -x
set -e

get_idt_ivshmem_opts(){
	local OPTS="-device idt-ntb-ivshmem,vectors=3,chardev=ivshmem,number=$1 -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem"
	if [ "$IVSHMEM_COMMON_OPTIONS_OVERRIDE" ]; then
		OPTS="$IVSHMEM_COMMON_OPTIONS_OVERRIDE"
	else
		IVSHMEM_COMMON_OPTIONS="$OPTS,$IVSHMEM_COMMON_OPTIONS"
	fi
	echo $OPTS
}

get_vm_opts(){
	local TELNEL_PORT=$((8000 + $1))
	local SSH_PORT=$((7000 + $1))
	local OPTS="-serial telnet::$TELNEL_PORT,server,nowait -net nic -net user,hostfwd=tcp::$SSH_PORT-:22"
	echo "$OPTS"
}

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

IVSHMEM_COMMON_OPTIONS_VM1=$(get_idt_ivshmem_opts 0)
IVSHMEM_COMMON_OPTIONS_VM2=$(get_idt_ivshmem_opts 1)

CMDLINE_COMMON_DEFAULT="root=/dev/vda rw ip=dhcp oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1"

if [ "$CMDLINE_COMMON_OVERRIDE" ]; then
	CMDLINE_COMMON="$CMDLINE_COMMON_OVERRIDE"
else
	CMDLINE_COMMON="$CMDLINE_COMMON_DEFAULT $CMDLINE_COMMON"
fi

COMMON_OPTIONS_DEFAULT="-usb -device usb-tablet -usb -device usb-kbd \
	-cpu IvyBridge -machine q35,i8042=off -smp 4 -m 256 \
	-nographic -monitor null"

VM1_OPTIONS_DEFAULT=$(get_vm_opts 1)
VM2_OPTIONS_DEFAULT=$(get_vm_opts 2)

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

if [ "$QEMU_SHM_SIZE" ]; then
	VM1_OPTIONS="$VM1_OPTIONS \
		-object memory-backend-file,id=mem,size=$((QEMU_SHM_SIZE / 2))M,mem-path=/dev/shm/qemu1,share=on \
		-machine memory-backend=mem -m $((QEMU_SHM_SIZE / 2))m"
	VM2_OPTIONS="$VM2_OPTIONS \
		-object memory-backend-file,id=mem,size=$((QEMU_SHM_SIZE / 2))M,mem-path=/dev/shm/qemu2,share=on \
		-machine memory-backend=mem -m $((QEMU_SHM_SIZE / 2))m"
fi

"$ivshmem_server" -m . -l 1M -n 3 -F &

"$qemu" \
	-drive file="$drive",if=virtio,format=raw \
	-kernel "$kernel" \
	-append "$CMDLINE_COMMON ivshmem_master" \
	$COMMON_OPTIONS \
	$IVSHMEM_COMMON_OPTIONS_VM1 \
	$VM1_OPTIONS &
(
	sleep 3 &&
	"$qemu" \
	-drive file="$drive2",if=virtio,format=raw \
	-kernel "$kernel2" \
	-append "$CMDLINE_COMMON" \
	$COMMON_OPTIONS \
	$IVSHMEM_COMMON_OPTIONS_VM2 \
	$VM2_OPTIONS &
)


wait

set +x

echo "All virtual machines exited."
