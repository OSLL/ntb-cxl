# QEMU IDT NTB Device Emulation

## Recent logs from the ntb_hw_idt driver

```
[    1.364334] e1000e 0000:00:02.0 eth0: (PCI Express:2.5GT/s:Width x1) 52:54:00:12:34:56
[    1.423544] ntb_hw_idt 0000:00:04.0: IDT 89HPES24NT6AG2 discovered
[    1.423745] ntb_hw_idt 0000:00:04.0: PCIe AER capability disabled
[    1.426801] ntb_hw_idt 0000:00:04.0: IDT NTB device is ready
```

## Current work done

The patches are located [here](../yocto_files/qemu-patches/)

### `0001-Changed-Vendor-Device-IDs-added-ivshmem_read_config-.patch`

The patch [`0001-Changed-Vendor-Device-IDs-added-ivshmem_read_config-.patch`](../yocto_files/qemu-patches/0001-Changed-Vendor-Device-IDs-added-ivshmem_read_config-.patch) contains the following changes:
* Vendor, device and class IDs changed
* Device is changed from PCI to PCIe
* Added function to handle read operations from PCIe configuration space
* Added initialization of MSI interrupts in the qemu device

### `0002-Added-debug-output-to-stderr.patch`

The patch [`0002-Added-debug-output-to-stderr.patch`](../yocto_files/qemu-patches/0002-Added-debug-output-to-stderr.patch) contains the following changes:
* Debug output enabled
* Changed debug output from `stdout` to `stderr`
* Added some extra debug prints

### `0003-Added-partially-support-for-ntb_pingpong-test.patch`

The patch [`0003-Added-partially-support-for-ntb_pingpong-test.patch`](../yocto_files/qemu-patches/0003-Added-partially-support-for-ntb_pingpong-test.patch) contains the following changes:
* The following topology is emulated:
  * The ports under indices `0` and `2` are in the partition with index `0`
  * Other ports and partitions are disabled
  * The port with index `0` is the local port to which guest is connected
  * The port with index `2` is the second PCIe domain
* Added emulation for the `GASAADDR`, `GASADAT`, `NTMTBLADDR` and `NTMTBLDATA` registers
* Added inbound and outbound message registers and their logic. Also added doorbell registers
* Added MSI interrupt support for the guest
* Added partial support for NTB interrupt logic

## Current problems

* The ping pong test is emulated in QEMU, i.e. the second guest is not used and the message value is incremented in the qemu device
* The rest of the IDT NTB device logic is not implemented:
  * Registers not used in the ping pong test
  * Interrupt logic with doorbells
  * Address translation and other NTB features

## Next work

Implementation of using ivshmem server and running pingpong on two guests
