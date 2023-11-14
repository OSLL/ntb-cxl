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

	if [ "$ARCH" = riscv64 ]; then
		if [ "$1" = 1 ]; then
			OPTS="$OPTS -bios $bios"
		elif [ "$2" = 2 ]; then
			OPTS="$OPTS -bios $bios2"
		fi
	fi

	echo "$OPTS"
}

. "$(dirname $0)"/common_variables.sh

cd "$YOCTO_WORK_DIR"/poky/build

ARCH="$NTB_CXL_ARCH"

qemu="$(find ./tmp/work/x86_64-linux/qemu-system-native -type f -name qemu-system-$ARCH | head -n1)"
ivshmem_server="$(find ./tmp/work/x86_64-linux/qemu-system-native -type f -name ivshmem-server | head -n1)"

vm1_dir=guest_1
vm2_dir=guest_2

drive=$vm1_dir/core-image-full-cmdline-qemu${ARCH}.ext4
drive2=$vm2_dir/core-image-full-cmdline-qemu${ARCH}.ext4
case "$ARCH" in
	x86_64)
		kernel=$vm1_dir/bzImage
		kernel2=$vm2_dir/bzImage
		;;
	riscv64)
		kernel=$vm1_dir/Image
		kernel2=$vm2_dir/Image
		bios=$vm1_dir/fw_jump.elf
		bios2=$vm2_dir/fw_jump.elf
		;;
	*)
		echo "error: invalid arch: $ARCH"
		return 1

esac

IVSHMEM_COMMON_OPTIONS_VM1=$(get_idt_ivshmem_opts 0)
IVSHMEM_COMMON_OPTIONS_VM2=$(get_idt_ivshmem_opts 1)

CMDLINE_COMMON_DEFAULT="root=/dev/vda rw ip=dhcp oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1"

if [ "$CMDLINE_COMMON_OVERRIDE" ]; then
	CMDLINE_COMMON="$CMDLINE_COMMON_OVERRIDE"
else
	CMDLINE_COMMON="$CMDLINE_COMMON_DEFAULT $CMDLINE_COMMON"
fi

COMMON_OPTIONS_DEFAULT="-smp 4 -m 256 -nographic -monitor null"
case "$ARCH" in
	x86_64)
		COMMON_OPTIONS_DEFAULT="$COMMON_OPTIONS_DEFAULT -cpu IvyBridge -machine q35"
		;;
	riscv64)
		COMMON_OPTIONS_DEFAULT="$COMMON_OPTIONS_DEFAULT -machine virt"
		;;
	*)
		echo "error: invalid arch: $ARCH"
		return 1
esac

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
