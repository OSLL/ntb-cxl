# NTB/CXL Bridge for InterVM communication

## Requirements
- [Docker](https://docs.docker.com/engine/install/)

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

Run `./run_container.sh --command=build` to build the project

## Run VM

To run VM use command: `./run_container.sh --command=run_vm`

User credentials to login into the vm:
- user: ```root```
- pswd: not set

## Run two connected VMs with `idt-ntb-ivshmem`

Run the following command: `./run_container.sh --command=run_vms`

It uses the `scripts/run_vms.sh` script to run `ivshmem-server` and virtual machines

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
$ ./run_container.sh --cmd=run_vm --qemu-map-ram-to-shm[=SIZE]
```
or
```ShellSessinon
$ ./run_container.sh --cmd=run_vms --qemu-map-ram-to-shm[=SIZE]
```

The `=SIZE` is optional. The default is `256`.
The value must be provided in mebibytes, without any suffix.

In case of a single VM, the shared memory will be at `/dev/shm/qemu`.
In case of two VMs, the shared memory files will be respectively at
`/dev/shm/qemu1` and `/dev/shm/qemu2`.

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

This is the basic usage.
Also, setting the peer_mask should work.
See [the kernel documentation](
https://docs.kernel.org/driver-api/ntb.html#ntb-tool-test-client-ntb-tool)
for the full usage.

The one thing that is not documented there are the message registers.
Currently, only passing `msg0` should work.
It works like `db` and `peer_db`.
Write to `peer0/msg0`, read from `msg0`.
