SUMMARY = "Build of NTRDMA external Linux kernel module"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=82fe63688974d81a0e4fa022cbc370d1"

inherit module

SRC_URI = "file://COPYING \
	   git://github.com/ntrdma/ntrdma-ext;branch=master;protocol=https \
	   "

S = "${WORKDIR}"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.

RRECOMENDS:{$PN} = "kernel-module-ntrdma"
RPROVIDES:${PN} += "kernel-module-ntrdma"
MACHINE_EXTRA_RRECOMMENDS += "kernel-module-ntrdma"

