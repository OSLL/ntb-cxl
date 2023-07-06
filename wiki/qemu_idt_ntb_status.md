# QEMU IDT NTB Device Emulation

## Recent logs from the ntb_hw_idt driver

```
[    1.388639] e1000e 0000:00:02.0 eth0: (PCI Express:2.5GT/s:Width x1) 52:54:00:12:34:56
[    1.468744] ntb_hw_idt 0000:00:04.0: IDT 89HPES24NT6AG2 discovered
[    1.468909] ntb_hw_idt 0000:00:04.0: PCIe AER capability disabled
[    1.469562] ntb_hw_idt 0000:00:04.0: No active peer found
[    1.471302] ntb_hw_idt 0000:00:04.0: IDT NTB device is ready
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

## Current problems

The log `No active peer found` means that there are no ports with the NT function configured.
From [comment in source ode](https://elixir.bootlin.com/linux/latest/source/drivers/ntb/hw/idt/ntb_hw_idt.c#L533):
`/* It's useless to have this driver loaded if there is no any peer */`.

NT ports are searched in the function [`idt_scan_ports`](https://elixir.bootlin.com/linux/latest/source/drivers/ntb/hw/idt/ntb_hw_idt.c#L480).

## Next work

Need to modify qemu device to emulate ports configs
