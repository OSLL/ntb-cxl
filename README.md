# NTB/CXL Bridge for InterVM communication

## Requirements
-- [Docker](https://docs.docker.com/engine/install/)

## Build VM
To build VM use command:
```
./run_container.sh
```
The full syntax is:
```
./run_container.sh build <bulid_directory>
```

This command will create folder ```build_vm_image``` and build vm image, Linux kernel image and all necessary dependences in this folder.

## Run VM

To run VM use command:
```
./run_qemu.sh
```

User credentials to login vm:
- user: ```root```
- pswd: is not set

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

As with `build`, both `enter_qemu_devenv` and `finish_qemu_devenv` support
specifying custom build directory,
and the same does `prepare_yocto.sh` and `qemu_enter_devenv.sh`.

When using host system, you may also try to use various other useful [devtool]
(https://docs.yoctoproject.org/kernel-dev/common.html#using-devtool-to-patch-the-kernel)
functions.
