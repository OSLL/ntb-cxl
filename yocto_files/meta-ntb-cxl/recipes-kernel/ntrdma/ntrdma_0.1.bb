SUMMARY = "Build of NTRDMA external Linux kernel module"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=864381d2980354cf8006157eae12d2a0"

inherit module

SRC_URI += "git://github.com/pim-pesochek/ntrdma-ext;protocol=https;branch=master"
# PV = "0.2-git${SRCPV}"
SRCREV = "55b7609126515beefc68789b255b9023b5161a54"

S = "${WORKDIR}/git"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.

EXTRA_OEMAKE += "KSRC=${STAGING_KERNEL_DIR}"

do_compile() {
unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS
	oe_runmake KERNEL_PATH=${STAGING_KERNEL_DIR}   \
		   KERNEL_VERSION=${KERNEL_VERSION}    \
		   CC="${KERNEL_CC}" LD="${KERNEL_LD}" \
		   AR="${KERNEL_AR}" \
	           O=${STAGING_KERNEL_BUILDDIR} \
		   KBUILD_EXTRA_SYMBOLS="${KBUILD_EXTRA_SYMBOLS}" \
		   modules
}

RPROVIDES:${PN} += "kernel-module-ntrdma"
