#!/bin/bash

COMMANDS=("server" "first" "second")
#  ivshmem-server -vF -n 2

if [ $# -lt 1 ]; then
    echo "You must specify the command for execution"
    exit 1
fi

if [[ " ${COMMANDS[*]} " == *" $1 "* ]]; then
    echo "Executing command $1"
else
    echo "Bad command. Available commands are [${COMMANDS[@]}]"
    exit 1
fi

if [ $1 == "server" ]; then
    ivshmem-server -vF -n 2
else
    BINS="$BUILD_PATH/bin_files"
    QEMU="$BUILD_PATH/poky/build/tmp/work/x86_64-linux/qemu-helper-native/1.0-r1/recipe-sysroot-native/usr/bin/qemu-system-x86_64"
    if [ $1 == "first" ]; then
        IMAGE_EXT4="$BINS/first-image.ext4"
        NUMBER=0
    else
        IMAGE_EXT4="$BINS/second-image.ext4"
        NUMBER=1
    fi
    # $BINS/qemu-system-x86_64
    $QEMU -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 -drive file=$IMAGE_EXT4,if=virtio,format=raw -cpu IvyBridge -machine q35 -smp 4 -m 256 -device idt-ntb-ivshmem,vectors=2,chardev=ivshmem,number=$NUMBER -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem -nographic  -kernel $BINS/bzImage -append 'root=/dev/vda rw console=ttyS0 console=ttyS1 oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1 ' -L $BINS/qemu_files
fi
