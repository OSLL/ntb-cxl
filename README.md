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
`./run_container --command=enter_qemu_devenv` should be executed.
After successful execution, the path to checkouted QEMU source repository
relative to selected build directory will be printed out

Command `enter_qemu_devenv` also copies all files from `qemu_src` directory
to working repository and automatically commits and squashes changes.

Make changes in QEMU and **commit them**. To run bash shell in docker container,
the `./run_container --command=shell` command can be used.

The `./run_container --command=finish_qemu_devenv` command should be used to
finish QEMU development. This will format patches from commits and copy them
into this repository to `yocto_files`.

When using host system, you may also try to use various other useful [devtool]
(https://docs.yoctoproject.org/kernel-dev/common.html#using-devtool-to-patch-the-kernel)
functions.
