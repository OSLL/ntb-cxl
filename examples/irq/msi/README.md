This example consists of a QEMU device and a kernel module.

To start it, do the following:
1. Add `msi-example-mod` to the `IMAGE_INSTALL:append` field in `yocto_files/configs/local.conf`.
2. Copy all files from the `qemu_src` subdir in that directory
to the `qemu_src` in the repository root.
3. Copy the local `msi-example-mod` directory
to `yocto_files/recipes-kernel/` in the repository root.
4. Enter and leave devenv to generate and copy patches.
5. Build.
