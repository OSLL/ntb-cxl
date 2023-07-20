FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:remove = "file://0001-qemu-Add-addition-environment-space-to-boot-loader-q.patch \
           file://0002-chardev-connect-socket-to-a-spawned-command.patch \
           file://0003-apic-fixup-fallthrough-to-PIC.patch \
           file://0004-configure-Add-pkg-config-handling-for-libgcrypt.patch \
           file://0005-qemu-Do-not-include-file-if-not-exists.patch \
           file://0006-qemu-Add-some-user-space-mmap-tweaks-to-address-musl.patch \
           file://0007-qemu-Determinism-fixes.patch \
           file://0008-tests-meson.build-use-relative-path-to-refer-to-file.patch \
           file://0009-Define-MAP_SYNC-and-MAP_SHARED_VALIDATE-on-needed-li.patch \
           file://0010-hw-pvrdma-Protect-against-buggy-or-malicious-guest-d.patch \
           file://0001-net-tulip-Restrict-DMA-engine-to-memories.patch \
           file://arm-cpreg-fix.patch \
           file://CVE-2022-3165.patch \
           file://CVE-2022-4144.patch \"

