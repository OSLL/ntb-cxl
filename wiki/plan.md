# Possible changes in qemu/kernel to implement ntb virtualization

## Qemu possible changes

We will use qemu 8.0. 
The qemu part requires communication between different qemu processes running virtual machines. As well as the implementation of a PCIe device, which will emulate the hardware necessary to work with NTB.

PCIe device in qemu can be implemented by self-written qemu device: [example](https://github.com/levex/kernel-qemu-pci/tree/master).
Main steps to write custom qemu device:
- [register qemu type](https://github.com/levex/kernel-qemu-pci/blob/31fc9355161b87cea8946b49857447ddd34c7aa6/qemu/hw/char/lev-pci.c#L142-L147) and set ```TYPE_PCI_DEVICE```
- init config for [ObjectClass](https://qemu.readthedocs.io/en/latest/devel/qom.html)
- in the ObjectClass implement [init function](https://github.com/levex/kernel-qemu-pci/blob/31fc9355161b87cea8946b49857447ddd34c7aa6/qemu/hw/char/lev-pci.c#L131C15-L131C30) with PCI device configuration (interruptions, register bars, allocate memmory)
- in the MemoryRegionOps register [operations](https://github.com/levex/kernel-qemu-pci/blob/31fc9355161b87cea8946b49857447ddd34c7aa6/qemu/hw/char/lev-pci.c#L83-L84) for PCI communication between qemu device and guest PCI device (main device logic)
- implement intervm communication (using any host IPC) in the memops read/write

The simplest way is modifying [ivshmem device](https://github.com/qemu/qemu/blob/master/hw/misc/ivshmem.c) (create another one basing on it). This device has already implemented intervm communication and qemu device registering.
Need to fix:
- interrupts for non-kvm virtualisation (now only [MSIX](https://github.com/qemu/qemu/blob/0618e72d64e434dd6f1bc38b107670474c49fb86/hw/misc/ivshmem.c#L731))
- PCIe device registering ([BAR registering](https://github.com/qemu/qemu/blob/0618e72d64e434dd6f1bc38b107670474c49fb86/hw/misc/ivshmem.c#L855), [device id, etc.](https://github.com/qemu/qemu/blob/0618e72d64e434dd6f1bc38b107670474c49fb86/hw/misc/ivshmem.c#L43-L50))
- check NTB-compatability

## Kernel possible changes
No need to change kernel drivers. We can use one of the implemented:
- [ntb_hw_intel](https://elixir.bootlin.com/linux/v6.1.38/source/drivers/ntb/hw/intel)
- [ntb_hw_amd](https://elixir.bootlin.com/linux/v6.1.38/source/drivers/ntb/hw/amd/ntb_hw_amd.c)
- [ntb_hw_idt](https://elixir.bootlin.com/linux/v6.1.38/source/drivers/ntb/hw/idt/ntb_hw_idt.c)
- [ntb_hw_epf](https://elixir.bootlin.com/linux/v6.1.38/source/drivers/ntb/hw/epf/ntb_hw_epf.c)
- [ntb_hw_switchtec](https://elixir.bootlin.com/linux/v6.1.38/source/drivers/ntb/hw/mscc/ntb_hw_switchtec.c)

As ```ntb_hw_intel```, ```ntb_hw_amd```, ```ntb_hw_switchtec``` have unknown logic for us (we have no any docs or specification) we will use ```ntb_hw_idt``` or ```ntb_hw_epf```.

```ntb_hw_epf``` requires implementation of the PCIe endpoint controller with [NTB configuration](https://dri.freedesktop.org/docs/drm/PCI/endpoint/pci-ntb-howto.html) in the qemu device. And then implement communication between two endpoints.
```use ntb_hw_idt``` has [NTB specification](https://www.renesas.com/eu/en/document/mah/89hpes32nt24bg2-device-user-manual). Based on the specification, it is possible to emulate the device to work without changes in the guest kernel.
