SUMMARY = "Netlink Library Suite"
DESCRIPTION = "${SUMMARY}"
LICENSE = "GPL-2.1"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c"

inherit autotools
inherit pkgconfig

SRC_URI += "git://github.com/thom311/libnl.git;protocol=https;branch=main"
SRCREV = "6b2533c02813ce21b27ea8318fbe1af95652a39e"

S = "${WORKDIR}/git"

DEPENDS = "bison-native"

