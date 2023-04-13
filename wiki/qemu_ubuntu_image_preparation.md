# Creating qemu image for NTB support
Create qemu image from root dir of this project

## Install Yocto project dependencies
```sudo apt install gawk wget git diffstat unzip texinfo gcc build-essential chrpath socat cpio python3 python3-pip python3-pexpect xz-utils debianutils iputils-ping python3-git python3-jinja2 libegl1-mesa libsdl1.2-dev pylint xterm python3-subunit mesa-common-dev zstd liblz4-tool```

## Download yocto

We will use **dunfell** LTS version of yocto
```
ROOT_PROJECT_PATH=$(pwd)
mkdir build_vm && cd build_vm
git clone git://git.yoctoproject.org/poky.git
cd poky
git checkout -t origin/dunfell -b my-dunfell
git pull
```

## Configure VM build
```
source oe-init-build-env
cp $ROOT_PROJECT_PATH/configs/yocto/local.conf ./conf/local.conf
```

## Configure linux kernel

```bitbake -c menuconfig virtual/kernel```

Enable options (*-option, not M-option for initializing modules on kernel boot) for NTB support:
```
-- Device Drivers
------ Non-Transparent Bridge support
--------- AMD Non-Transparent Bridge support
--------- IDT PCIe-switch Non-Transparent Bridge support
--------- Intel Non-Transparent Bridge support
--------- MicroSemi Switchtec Non-Transparent Bridge Support
--------- NTB Ping Pong Test Client
--------- NTB Debugging Tool Test Client
--------- NTB RAW Perf Measuring Tool
--------- NTB Transport Client
------ Network device support
--------- Virtual Ethernet over NTB Transport
```
Save config and exit

## Compile image
```
bitbake -c compile -f virtual/kernel
bitbake -c deploy virtual/kernel
bitbake core-image-full-cmdline
```

## Run qemu with PCI-to-PCI bridge
```runqemu qemux86-64 nographic qemuparams="-device pci-bridge,id=bridge0,chassis_nr=1"```

User: root

Password is not set

## Check kernel modules
Run: ```dmesg``` to see the kernel logs.

And you will see something like that:
```
[    3.874155] AMD(R) PCI-E Non-Transparent Bridge Driver 1.0
[    3.874338] IDT PCI-E Non-Transparent Bridge Driver 2.0
[    3.874577] Intel(R) PCI-E Non-Transparent Bridge Driver 2.0
[    3.875051] Software Queue-Pair Transport over NTB, version 4
```

```lsmod``` doesn`t show any modules.

```find / | grep ntb``` will give:

```
/sys/kernel/debug/ntb_transport
/sys/kernel/debug/ntb_perf
/sys/kernel/debug/ntb_tool
/sys/kernel/debug/ntb_pingpong
/sys/kernel/debug/ntb_hw_intel
/sys/kernel/debug/ntb_hw_idt
/sys/kernel/debug/ntb_hw_amd
/sys/bus/ntb_transport
/sys/bus/ntb_transport/uevent
/sys/bus/ntb_transport/drivers_autoprobe
/sys/bus/ntb_transport/devices
/sys/bus/ntb_transport/drivers_probe
/sys/bus/ntb_transport/drivers
/sys/bus/pci/drivers/ntb_hw_amd
/sys/bus/pci/drivers/ntb_hw_amd/uevent
/sys/bus/pci/drivers/ntb_hw_amd/bind
/sys/bus/pci/drivers/ntb_hw_amd/new_id
/sys/bus/pci/drivers/ntb_hw_amd/remove_id
/sys/bus/pci/drivers/ntb_hw_amd/unbind
/sys/bus/pci/drivers/ntb_hw_amd/module
/sys/bus/pci/drivers/ntb_hw_intel
/sys/bus/pci/drivers/ntb_hw_intel/uevent
/sys/bus/pci/drivers/ntb_hw_intel/bind
/sys/bus/pci/drivers/ntb_hw_intel/new_id
/sys/bus/pci/drivers/ntb_hw_intel/remove_id
/sys/bus/pci/drivers/ntb_hw_intel/unbind
/sys/bus/pci/drivers/ntb_hw_intel/module
/sys/bus/pci/drivers/ntb_hw_idt
/sys/bus/pci/drivers/ntb_hw_idt/uevent
/sys/bus/pci/drivers/ntb_hw_idt/bind
/sys/bus/pci/drivers/ntb_hw_idt/new_id
/sys/bus/pci/drivers/ntb_hw_idt/remove_id
/sys/bus/pci/drivers/ntb_hw_idt/unbind
/sys/bus/pci/drivers/ntb_hw_idt/module
/sys/bus/ntb
/sys/bus/ntb/uevent
/sys/bus/ntb/drivers_autoprobe
/sys/bus/ntb/devices
/sys/bus/ntb/drivers_probe
/sys/bus/ntb/drivers
/sys/bus/ntb/drivers/ntb_transport
/sys/bus/ntb/drivers/ntb_transport/uevent
/sys/bus/ntb/drivers/ntb_transport/bind
/sys/bus/ntb/drivers/ntb_transport/unbind
/sys/bus/ntb/drivers/ntb_pingpong
/sys/bus/ntb/drivers/ntb_pingpong/uevent
/sys/bus/ntb/drivers/ntb_pingpong/bind
/sys/bus/ntb/drivers/ntb_pingpong/unbind
/sys/bus/ntb/drivers/ntb_tool
/sys/bus/ntb/drivers/ntb_tool/uevent
/sys/bus/ntb/drivers/ntb_tool/bind
/sys/bus/ntb/drivers/ntb_tool/unbind
/sys/bus/ntb/drivers/ntb_perf
/sys/bus/ntb/drivers/ntb_perf/uevent
/sys/bus/ntb/drivers/ntb_perf/bind
/sys/bus/ntb/drivers/ntb_perf/unbind
/sys/module/ntb_transport
/sys/module/ntb_transport/uevent
/sys/module/ntb_transport/parameters
/sys/module/ntb_transport/parameters/transport_mtu
/sys/module/ntb_transport/parameters/use_dma
/sys/module/ntb_transport/parameters/max_mw_size
/sys/module/ntb_transport/parameters/copy_bytes
/sys/module/ntb_transport/parameters/max_num_clients
/sys/module/ntb_transport/version
/sys/module/ntb_pingpong
/sys/module/ntb_pingpong/uevent
/sys/module/ntb_pingpong/parameters
/sys/module/ntb_pingpong/parameters/unsafe
/sys/module/ntb_pingpong/parameters/delay_ms
/sys/module/ntb_pingpong/version
/sys/module/ntb_hw_amd
/sys/module/ntb_hw_amd/uevent
/sys/module/ntb_hw_amd/version
/sys/module/ntb_hw_amd/drivers
/sys/module/ntb_hw_amd/drivers/pci:ntb_hw_amd
/sys/module/ntb_tool
/sys/module/ntb_tool/uevent
/sys/module/ntb_tool/version
/sys/module/ntb_perf
/sys/module/ntb_perf/uevent
/sys/module/ntb_perf/parameters
/sys/module/ntb_perf/parameters/chunk_order
/sys/module/ntb_perf/parameters/use_dma
/sys/module/ntb_perf/parameters/total_order
/sys/module/ntb_perf/parameters/max_mw_size
/sys/module/ntb_perf/version
/sys/module/ntb_hw_intel
/sys/module/ntb_hw_intel/uevent
/sys/module/ntb_hw_intel/parameters
/sys/module/ntb_hw_intel/parameters/b2b_mw_share
/sys/module/ntb_hw_intel/parameters/xeon_b2b_usd_bar2_addr64
/sys/module/ntb_hw_intel/parameters/xeon_b2b_usd_bar4_addr64
/sys/module/ntb_hw_intel/parameters/xeon_b2b_dsd_bar2_addr64
/sys/module/ntb_hw_intel/parameters/xeon_b2b_dsd_bar4_addr64
/sys/module/ntb_hw_intel/parameters/xeon_b2b_usd_bar4_addr32
/sys/module/ntb_hw_intel/parameters/xeon_b2b_usd_bar5_addr32
/sys/module/ntb_hw_intel/parameters/xeon_b2b_dsd_bar4_addr32
/sys/module/ntb_hw_intel/parameters/xeon_b2b_dsd_bar5_addr32
/sys/module/ntb_hw_intel/parameters/b2b_mw_idx
/sys/module/ntb_hw_intel/version
/sys/module/ntb_hw_intel/drivers
/sys/module/ntb_hw_intel/drivers/pci:ntb_hw_intel
/sys/module/ntb_hw_switchtec
/sys/module/ntb_hw_switchtec/uevent
/sys/module/ntb_hw_switchtec/parameters
/sys/module/ntb_hw_switchtec/parameters/use_lut_mws
/sys/module/ntb_hw_switchtec/parameters/max_mw_size
/sys/module/ntb_hw_switchtec/version
/sys/module/ntb_hw_idt
/sys/module/ntb_hw_idt/uevent
/sys/module/ntb_hw_idt/version
/sys/module/ntb_hw_idt/drivers
/sys/module/ntb_hw_idt/drivers/pci:ntb_hw_idt
/sys/module/ntb_netdev
/sys/module/ntb_netdev/uevent
/sys/module/ntb_netdev/version
/sys/module/ntb
/sys/module/ntb/uevent
/sys/module/ntb/version

```



