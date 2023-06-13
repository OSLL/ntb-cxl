DESCRIPTION = "Simple helloworld application"
SECTION = "examples"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ivshmem_common.h \
           file://src/userctl.c"

S = "${WORKDIR}"

do_compile() {
	${CC} ${LDFLAGS} src/userctl.c -o userctl -lrt
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 userctl ${D}${bindir}
}
