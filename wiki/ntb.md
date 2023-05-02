# Non-Transparent Bridge

## What to read about NTB

* [Presentation about NTB](https://events.static.linuxfound.org/sites/events/files/slides/Linux%20NTB_0.pdf)
* [Kernel presentation about NTB](https://events.static.linuxfound.org/sites/events/files/slides/Linux%20NTB_0.pdf)
* [Kernel documentation about NTB](https://www.kernel.org/doc/Documentation/ntb.txt )
* [Using Non-transparent Bridging in PCI Express Systems](https://docs.broadcom.com/doc/12353428)
* [Some usefull presentation about NTB](https://www.snia.org/educational-library?search=NTB&field_edu_content_type_tid=All&field_assoc_event_name_tid=All&field_release_date_value_2%5Bvalue%5D%5Byear%5D=&field_focus_areas_tid=All&field_author_tid=&field_release_date_value=All&items_per_page=20&captcha_sid=2333586&captcha_token=6536b11fe622a8fcef53f08d333dc38b&captcha_cacheable=1)
* [Simple NTB usecase](https://www.simula.no/file/s9709-smartio-gtc2019pdf/download)
* [NTB hardware requirements](https://github.com/jonmason/ntb/wiki)

## NTB examples links

* [Nvidia](https://docs.nvidia.com/drive/drive_os_5.1.6.1L/nvvib_docs/index.html#page/DRIVE_OS_Linux_SDK_Development_Guide/System%20Programming/sys_components_non_transparent_bridging.html) example: two Xaviers on Pegasus board are connected with PCI
* [NTB with PCIe endpoints](https://lpc.events/event/4/contributions/395/attachments/284/481/Implementing_NTB_Controller_Using_PCIe_Endpoint_-_final.pdf)([Corresponding video](https://www.youtube.com/watch?v=dLKKxrg5-rY)): Three devices with PCIe endpoints are connected, where one is act as NTB switch. Implemented via [PCI NTB Endpoint Function](https://docs.kernel.org/PCI/endpoint/pci-ntb-howto.html)
  * [About PCI endpoint framework](https://docs.kernel.org/PCI/endpoint/index.html)
* [VirtIO based communication between two hosts](https://lpc.events/event/7/contributions/849/attachments/642/1175/Virtio_for_PCIe_RC_EP_NTB.pdf)
* [Non-standart implementation of NTB driver for linux](https://doc.dpdk.org/guides/rawdevs/ntb.html)
