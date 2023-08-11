# NTB/CXL Bridge for InterVM communication

## Requirements
- [Docker](https://docs.docker.com/engine/install/)

## Using container

`run_container.sh` is a unified starting point for the Docker container. The general syntax is:
```
./run_container.sh <command> <bulid_directory>
```
where `<command>` is currently one of `build`, `enter_qemu_devenv` or `finish_qemu_devenv`.
If no command is given, `build` is assumed.

## Build VM

The `build` command will create folder ```build_vm_image``` and build vm image, Linux kernel image and all necessary dependences in this folder.

## Run VM

To run VM use command:
```
./run_container.sh run_vm
```

User credentials to login into the vm:
- user: ```root```
- pswd: not set

## Run two connected VMs
```
./run_container.sh run_vms
```
It uses the `scripts/run_vms.sh` script,
which can also be used outside of the container.

QEMU options can be customized via cli options:
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

*`run_container.sh` script has one more argument: `--host-build-dir=DIR`.
It represents relative path to the build directory that is mapped to the
container. This argument is not passed to the container's entrypoint*

## Developing QEMU

You can do it both on your host system and with Docker container.

On your host system, after making sure you have already run `prepare_yocto.sh`,
execute `qemu_enter_devenv.sh`.

If using Docker, run `./run_container.sh enter_qemu_devenv`.

In both cases script will print path to checked out QEMU sources repository
relative to selected build directory.

`qemu_enter_devenv.sh` script also copies all files from qemu_src to working repository
and automatically commits and squashes changes.

After making changes, commit them.

When you're done, run `qemu_finish_devenv.sh` (or `./run_container.sh finish_qemu_devenv`).
This will format patches from commits and copy them into this repository to `yocto_files`.

For any changes to take effect, qemu should be rebuilt
(`./run_container.sh build` or `scripts/prepare_yocto.sh && scripts/build_yocto.sh`).

As with `build`, both `enter_qemu_devenv` and `finish_qemu_devenv` support
specifying custom build directory,
and the same does `prepare_yocto.sh` and `qemu_enter_devenv.sh`.

When using host system, you may also try to use various other useful [devtool]
(https://docs.yoctoproject.org/kernel-dev/common.html#using-devtool-to-patch-the-kernel)
functions.
