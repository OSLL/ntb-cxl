# NTB/CXL Bridge for InterVM communication

## Project description
This project implements the virtualization of NTB functionality of the IDT 89HPES24NT6AG2 PCI Express Switch device in QEMU. The project has fixed the Linux Kernel version 6.1 and the QEMU version v8.0.2. The goal of the project is to implement a virtual NTB connection between 2 guest machines. The target scenario is the use of RDMA technology through the NTRDMA driver.

Implemented at the moment:
- VM guest images are based on `core-image-full-cmdline`. Added to the image: `ntb_hw_idt`, `ntb`, `ntb_transport`, `ntb_tool` as a module, `ntb_perf` as a module, `ntrdma` as a module and necessary dependencies.
- Added `idt-ntb-ivshmem` device to QEMU.
- Support for NTB registers and functionality:
  - NTB link functions
  - Message registers
  - Doorbell registers
  - Interrupts and interrupt masking
  - Memory window translation
  - Implementation of mw translation in DIR mode
  - Direct read/write into DMA VM space be i/o operations into mw
  - Added unsupported by IDT 89HPES24NT6AG2 NTB operations: `mw_set_trans` and `mw_clear_trans` functions (Necessary for `ntb_perf`). So the `ntb_hw_idt` driver was modified.
- PCIe port 0 and port 2 are configured
- Only one partition is implemented


## Known issues
- `ntb_perf` test with `use_dma` flag failes (due to the lack of implementation of the DMA controller)
- `ntrdma` init failes with error (for the same reason as the previous point)
- if `run_vm` or `run_vms` failes with dynamic link error you need to uncommnet string: `cp $(find ./tmp/work/x86_64-linux/qemu-system-native -type f -name qemu-system-x86_64 | sed -n 2p) $(find ./tmp/work/x86_64-linux/qemu-system-native -type f -name qemu-system-x86_64 | sed -n 1p)` in the `scripts/build_yocto.sh` and rerun qemu build by the command `./run_container.sh --command=build --build=qemu`


## Requirements
- [Docker](https://docs.docker.com/engine/install/)

## Quick start
1) Build the project in the docker container:
```
./run_container.sh --command=enter_qemu_devenv
./run_container.sh --command=finish_qemu_devenv
./run_container.sh --command=build
```
2) Run VMs:
```
./run_container.sh --command=run_vms --qemu-map-ram-to-shm=1000
```
3) Connect to the VM1 and VM2 in the another teminals `ssh root@localhost -p7001` and `ssh root@localhost -p7002`

4) Run `ntb_perf` module to test the data transmition on both VMs: `modprobe ntb_perf chunk_order=8 total_order=20`. `chunk_order` is data chunk order [2^n] to transfer, `total_order` is the total data order [2^n] to transfer (a large total order can lead to a MW allocation error due to the BAR size limit and memory limitations of the virtual machine).

5) Check connection info `cat /sys/kernel/debug/ntb_perf/0000\:00\:03.0/info`.

Example:
```
root@qemux86-64:~# cat /sys/kernel/debug/ntb_perf/0000\:00\:03.0/info 
    Performance measuring tool info:

Local port 2, Global index 1
Test status: idle
Port 0 (0), Global index 0:
	Link status: up
	Out buffer addr 0x00000000e4157aba
	Out buff phys addr 0x00000000fe900000[p]
	Out buffer size 0x0000000000100000
	Out buffer xlat 0x0000000004b00000[p]
	In buffer addr 0x00000000a1d2914a
	In buffer size 0x0000000000100000
	In buffer xlat 0x0000000004b00000[p]
```

6) Run test by `echo 0 > /sys/kernel/debug/ntb_perf/0000\:00\:03.0/run ` and check the result by `cat /sys/kernel/debug/ntb_perf/0000\:00\:03.0/run`.

Example:
```
root@qemux86-64:~# echo 0 > /sys/kernel/debug/ntb_perf/0000\:00\:03.0/run 
root@qemux86-64:~# cat /sys/kernel/debug/ntb_perf/0000\:00\:03.0/run 
    Peer 0 test statistics:
0: copied 1048576 bytes in 1325470 usecs, 0 MBytes/s
```
## Usage

### Using docker

`run_container.sh` is a unified starting point for the Docker container.
The general syntax is:
```
./run_container.sh [OPTIONS]...
```
Run `./run_container.sh -h` to see the help.

*NOTE:* `run_container.sh` has one extra option -- `--host-build-dir=DIR`.
It represents relative path to the build directory that is mapped to the
container. This argument is not passed to the container's entrypoint. By
default the `./build_vm_image` directory is used.

### Using locally

First of all, install necessary [dependencies](https://docs.yoctoproject.org/brief-yoctoprojectqs/index.html#build-host-packages)
for yocto project.

Then use `./scripts/dispatch_docker_command.sh` as entrypoint. This is
equivalent to using `run_container.sh` script. The rest of the README assumes
that the **docker case is used**

## Build project

First of all to add the `idt-ntb-ivshmem` device to the qemu it is necessary to create a patch from local source files:
```
./run_container.sh --command=enter_qemu_devenv
./run_container.sh --command=finish_qemu_devenv
```

After `finish_qemu_devenv` command patch will be created and appended to the `meta-ntb-cxl` layer.

Run `./run_container.sh --command=build` to build the project.

## Run VM

To run VM use command: `./run_container.sh --command=run_vm`

User credentials to login into the vm:
- user: ```root```
- pswd: not set

## Run two connected VMs with `idt-ntb-ivshmem`

Run the following command: `./run_container.sh --command=run_vms --qemu-map-ram-to-shm=1000`. Flag `--qemu-map-ram-to-shm=1000` is necessary for memory window i/o operations to directly read/write into vm memory.

It uses the `scripts/run_vms.sh` script to run `ivshmem-server` and virtual machines. 

QEMU options can be customized via CLI options:
- `--ivshmem-common-opts`
- `--cmdline-common`
- `--common-opts`
- `--vm1-opts`
- `--vm2-opts`

The default behavior is to append whatever is specified in that variables
to default values.
To override the default value instead, suffix the option with `-override`,
like `common-opts-override`. Run `--help` for more information

To see default values of that options, refer to the script itself.

To connect to the VMs can be used telnet or ssh protocol:
- telnet port 8001 for VM1 and port 8002 for VM2
- ssh port 7001 for VM1 and port 7002 for VM2

## QEMU development

QEMU development can be done both on a host system and with Docker

Before starting, the project should be built and the command
`./run_container.sh --command=enter_qemu_devenv` should be executed.
After successful execution, the path to checkouted QEMU source repository
relative to selected build directory will be printed out

Command `enter_qemu_devenv` also copies all files from `qemu_src` directory
to working repository and automatically commits and squashes changes.

Make changes in QEMU and **commit them**. To run bash shell in docker container,
the `./run_container --command=shell` command can be used.

The `./run_container --command=finish_qemu_devenv` command should be used to
finish QEMU development. This will format patches from commits and copy them
into this repository to `yocto_files`. When QEMU development is finished,
QEMU rebuild is required. This can be done with
`./run_container.sh --command=build --build=qemu` command

When using host system, you may also try to use various other useful [devtool]
(https://docs.yoctoproject.org/kernel-dev/common.html#using-devtool-to-patch-the-kernel)
functions.

### QEMU debugging

It might be useful to be able to read/write to a VM memory directly.

It's possible using one the following commands:
```ShellSessinon
$ ./run_container.sh --command=run_vm --qemu-map-ram-to-shm[=SIZE]
```
or
```ShellSessinon
$ ./run_container.sh --command=run_vms --qemu-map-ram-to-shm[=SIZE]
```

The `=SIZE` is optional. The default is `256`.
The value must be provided in mebibytes, without any suffix.

In case of a single VM, the shared memory will be at `/dev/shm/qemu`.
In case of two VMs, the shared memory files will be respectively at
`/dev/shm/qemu1` and `/dev/shm/qemu2`.

Also, in case of 2 VMs,
each VM's allocated memory is equal to a half of the total passed memory size.

If using docker, to interact with shared memory you must run a shell inside a container:
```ShellSession
$ docker exec -it <containername> bash
docker$ dd if=/dev/shm/qemu bs=1 count=8
docker$ ...
```

## Testing the IDT NTB QEMU device

### `ntb_pingpong`

```ShellSession
$ ./run_container.sh --command=run_vms
```
QEMU should report that something is transferred over out NTB device.
Also, the counter in debugfs should increase over time:
```ShellSession
$ ssh root@localhost -p7001
# cat /sys/kernel/debug/ntb_pingpong/*/count
```

### `ntb_tool`

```ShellSession
$ ./run_container.sh --command=run_vms --cmdline-common="initcall_blacklist=pp_init"
```
(needed to load `ntb_tool` instead of `ntb_pingpong`, both are built-in modules currently)

#### Doorbell register

VM1:
```ShellSession
$ ssh root@localhost -p7001
# cd /sys/kernel/debug/ntb_tool/*
# echo 's 0xdeadbeef' >peer_db
```

VM2:
```ShellSession
$ ssh root@localhost -p7002
# cd /sys/kernel/debug/ntb_tool/*
# cat db
0xdeadbeef
```

Also, setting the peer_mask should work.

#### Message registers

VM1:
```ShellSession
# echo 0xdeadbeef >peer0/msg0
```

VM2:
```ShellSession
# cat msg0
0xdeadbeef<-0
```

All message registers (`msg[0-3]`) should work.

Subsequent writes should not be performed if the MSGSTS of the target peer
was not cleared (OUTMSGx is set and an interrupt is sent to host which
attempts to write):

```
IVSHMEM: Refusing to write to msg register, INMSGSTS0 is non-zero (vm1 0x10000)
```

INMSGSTSx fields of MSGSTS are set on a write to INMSGx
and should be unset by the client software on the peer (e.g. `ntb_pingpong`).
With `ntb_tool` it can be achieved with the following:
```ShellSession
# cat msg_sts
0x10000
# echo 'c 0xffffffff' >msg_sts
# cat msg_sts
0x0
```

#### Additional sources

See [the kernel documentation](
https://docs.kernel.org/driver-api/ntb.html#ntb-tool-test-client-ntb-tool)
for the full usage.
Even more functionality is described in [the source code](
https://elixir.bootlin.com/linux/v6.1.50/source/drivers/ntb/test/ntb_tool.c#L54).

### `ntb_hw_idt` debugfs node

The driver has its own debugfs node with some useful info:
```ShellSession
# cat /sys/kernel/debug/ntb_hw_idt/info\:0000\:00\:03.0
```

It can be used to verify that VMs have correct ports.

Also it's useful for viewing `msg_mask`, which is write-only in `ntb_tool`.
