# Installing Yocto project
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
$ROOT_PROJECT_PATH/configs/yocto/local.conf ./conf/local.conf
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

## Run qemu
```runqemu qemux86-64 nographic```

User: root

Password is not set




