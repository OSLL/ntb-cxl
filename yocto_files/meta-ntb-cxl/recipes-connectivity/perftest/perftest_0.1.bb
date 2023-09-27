SUMMARY = "Perftest from Linux RDMA library"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://COPYING;md5=9310aaac5cbd7408d794745420b94291"

inherit autotools

SRC_URI += "git://github.com/linux-rdma/perftest.git;protocol=https;branch=master"
SRCREV = "9411e12bad57fb36754e6f77b9ce92a7af1eb22a"

S = "${WORKDIR}/git"

DEPENDS = "rdma-core pciutils"
