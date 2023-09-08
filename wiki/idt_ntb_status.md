# QEMU IDT NTB Device Emulation Status

## Global status

The IDT NTB is implemented as a new qemu device with name `idt-ntb-ivshmem`.
The device is based on the `ivshmem-doorbell` device. Communication between
two virtual machines has been added. For now, only ping-pong test functionality
is supported (module `ntb_pingpong`)

## What is done

Implemented features:
* Vendor, device and class IDs changed
* Device is changed from PCI to PCIe
* Functions to handle read operations from PCIe configuration space and BAR0
* MSI interrupt support for the guest
* The following topology is emulated:
  * The ports under indices `0` and `2` are in the partition with index `0`
  * Other ports and partitions are disabled
  * The port with index `0` is the local port to which guest is connected
  * The port with index `2` is the second PCIe domain
* Emulation of the `GASAADDR`, `GASADAT`, `NTMTBLADDR` and `NTMTBLDATA` registers
* Partial support for:
  * inbound and outbound message registers
  * doorbell registers
  * NTB interrupt logic
* Connection of two guests using ivshmem-server

## Next work

Implementation of the rest of the device functions

## How to run

1. Start a ivshmem-server with at least two vectors and at least 48 bytes of shared memory.
2. Run the first guest with the `number=0` argument.
2. Run the second guest with the `number=1` argument.
4. Run `dmesg` and find the following lines on each guest:
```
[    1.364334] e1000e 0000:00:02.0 eth0: (PCI Express:2.5GT/s:Width x1) 52:54:00:12:34:56
[    1.423544] ntb_hw_idt 0000:00:04.0: IDT 89HPES24NT6AG2 discovered
[    1.423745] ntb_hw_idt 0000:00:04.0: PCIe AER capability disabled
[    1.426801] ntb_hw_idt 0000:00:04.0: IDT NTB device is ready
```
5. Congratulation! You have two virtual machines connected using NTB.

### Example commands

**Assuming you have built qemu with the `idt-ntb-ivshmem` device, built the linux kernel and system image**

1. Run ivshmem-server in a separate terminal: `ivshmem-server -vF -n 2` (Note, it should be install, goes together with `qemu-system` package)
2. Run the first guest in a separate terminal: `/path/to/qemu-system-x86_64 -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 -drive file=/path/to/first-rootfs.ext4,if=virtio,format=raw -cpu IvyBridge -machine q35 -smp 4 -m 256 -device idt-ntb-ivshmem,vectors=2,chardev=ivshmem,number=0 -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem -nographic  -kernel /path/to/kernel -append 'root=/dev/vda rw console=ttyS0 console=ttyS1 oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1 '`
3. Run the first guest in a separate terminal: `/path/to/qemu-system-x86_64 -object rng-random,filename=/dev/urandom,id=rng0 -device virtio-rng-pci,rng=rng0 -drive file=/path/to/second-rootfs.ext4,if=virtio,format=raw -cpu IvyBridge -machine q35 -smp 4 -m 256 -device idt-ntb-ivshmem,vectors=2,chardev=ivshmem,number=1 -chardev socket,path=/tmp/ivshmem_socket,id=ivshmem -nographic  -kernel /path/to/kernel -append 'root=/dev/vda rw console=ttyS0 console=ttyS1 oprofile.timer=1 tsc=reliable no_timer_check rcupdate.rcu_expedited=1 '`
4. Run `cat /sys/kernel/debug/ntb_pingpong/0000\:00\:04.0/count ` command in one of the guest. You'll see the number of successful transmissions
5. After a while, run the previous command again and you will see that the number has increased
