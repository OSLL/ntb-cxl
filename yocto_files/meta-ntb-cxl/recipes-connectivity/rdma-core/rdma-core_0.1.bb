SUMMARY = "RDMA core library"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.0"
LIC_FILES_CHKSUM = "file://COPYING.GPL2;md5=b234ee4d69f5fce4486a80fdaf4a4263"

inherit pkgconfig
inherit cmake

SRC_URI += "git://github.com/linux-rdma/rdma-core.git;protocol=https;branch=stable-v47"
SRCREV = "8c45cd95bbbf7c7bd7e715edb464e0d25f56941a"

S = "${WORKDIR}/git"

DEPENDS = "libnl"

EXTRA_OECMAKE="-DNO_MAN_PAGES=1"
