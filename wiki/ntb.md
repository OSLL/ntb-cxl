# Non-Transparent Bridge

## What to read about NTB

* [Presentation about NTB](https://events.static.linuxfound.org/sites/events/files/slides/Linux%20NTB_0.pdf)
* [Kernel documentation about NTB](https://events.static.linuxfound.org/sites/events/files/slides/Linux%20NTB_0.pdf)
* [Using Non-transparent Bridging in PCI Express Systems](https://docs.broadcom.com/doc/12353428)

## NTB examples links

* [Nvidia](https://docs.nvidia.com/drive/drive_os_5.1.6.1L/nvvib_docs/index.html#page/DRIVE_OS_Linux_SDK_Development_Guide/System%20Programming/sys_components_non_transparent_bridging.html) example: two Xaviers on Pegasus board are connected with PCI
* [NTB with PCIe endpoints](https://lpc.events/event/4/contributions/395/attachments/284/481/Implementing_NTB_Controller_Using_PCIe_Endpoint_-_final.pdf)([Corresponding video](https://www.youtube.com/watch?v=dLKKxrg5-rY)): Three devices with PCIe endpoints are connected, where one is act as NTB switch. Implemented via [PCI NTB Endpoint Function](https://docs.kernel.org/PCI/endpoint/pci-ntb-howto.html)
  * [About PCI endpoint framework](https://docs.kernel.org/PCI/endpoint/index.html)
