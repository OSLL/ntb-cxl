# Implementation NTB with ivshmem

## Types of ivshmem

QEMU currently implements two types of InterVM shared memory:
1. Plain memory:
    * On the host it is a normal shared memory
    * In the VM, this is a PCI device with the resource
    * In the VM, the PCI device can be mapped with `mmap` or `pci_iomap` to access the shared memory
    * _No other features are present_
2. Doorbell:
    * On the host, a server is raised to send interruptions between VMs
    * In the VM it is a PCI device with resource that can be used as plain ivshmem
    * In the VM, interrupts can be set with a PCI device to handle events
    * _Works only with KVM ([see the chapter Doorbell KVM](#Doorbell-KVM))_

## Doorbell KVM

Interrupts in ivshmem-doorbell are implemented with MSI-X interrupts. The following functions are used to initialize interrupts in QEMU:
1. In register function ivshmem "classes" are set ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L1131))
2. `ivshmem_common_info` "class" contains function `ivshmem_common_class_init` for setup ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L1009))
3. In function `ivshmem_common_class_init`, function `ivshmem_common_realize` is set for the PCI device ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L992))
4. In function `ivshmem_common_realize`, the function `ivshmem_recv_setup` is called synchronously to receive a setup message from the server ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L878))
5. In function `ivshmem_recv_setup`, function `process_msg` is called ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L687)). Also this function is called from `ivshmem_read` ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L604))
6. In function `process_msg`, function `process_msg_connect` is called ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L572))
7. In function `process_msg_connect`, funciton `setup_interrupt` is used to setup interrupts ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L543))
8. In funciton `setup_interrupt`, function `ivshmem_add_kvm_msi_virq` is used for irq implementation, which uses KVM ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L460)). For eventfd implementation, function `watch_vector_notifier` is used ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L457))
9. In function `watch_vector_notifier`, the fd handler is set with function `ivshmem_vector_notify` ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L351))
10. In function `ivshmem_vector_notify`, function `msix_notify` is used for interrupt notifing ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/misc/ivshmem.c#L268))
11. In function `msix_notify`, function `msi_send_message` is called  ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/pci/msix.c#L533))
12. Funciton `msi_send_message` calls `(PCIDevice *) dev->msi_trigger` ([code](https://github.com/qemu/qemu/blob/stable-7.2/hw/pci/msi.c#L378))
13. ***What is next?***


***On RISCV interrupts are not initialized, error: invalid argumnets, cannot allocate interrupt lines***

## Roadmap

1. Support for six BARs for ivshmem device. Only three are currently in use
2. Interrupt support. There are two ways:
    1. Implementation of ivshmem-doorbel without KVM. Requires modification of existing qemu device
    2. Implementation on plain memory, i.e. implementation of the synchronization mechanism in the guest driver. (Other options?)
3. Guest driver with NTB interface to interact with ivshmem PCI device
