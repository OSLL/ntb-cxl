# Guidelines for qemu version update in Yocto project

1) Firstly we need to set the version in the config of yocto project: ```meta/conf/distro/include/tcmode-default.inc```
2) Change names of bb layer files: ```meta/recipes-devtools/qemu/qemu_<ver>.bb```, ```meta/recipes-devtools/qemu/qemu-native_<ver>.bb```, ```meta/recipes-devtools/qemu/qemu-system-native_<ver>.bb``` from old version to new version which is set in the point above
3) Fix ```meta/recipes-devtools/qemu/qemu.inc``` file. Need to change **SRC_URI[sha256sum]** and delete all unnecessary patches from **SRC_URI**

Example commits of version updates:
- [7.1->7.2](https://github.com/yoctoproject/poky/commit/9caff14abbb742e5083056b899ee6fc0a5fba8f3)
- [7.2->8.0](https://github.com/yoctoproject/poky/commit/fb8d21e73fbe3692bc1aa3455f957eb26c6b5ea8)

**Important**: it may be necessary to make changes in accordance with a specific version of qemu in layers and patches.

