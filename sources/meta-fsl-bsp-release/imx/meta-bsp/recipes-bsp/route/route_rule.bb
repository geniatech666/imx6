DESCRIPTION = "route_rule"
SECTION = "libs"
LICENSE = "MIT"
PV = "3"
PR = "r0"

SRC_URI = " \
          file://route_rule.c \
          file://makefile \
          "
TARGET_CC_ARCH += "${LDFLAGS}"

LIC_FILES_CHKSUM = "file://route_rule.c;md5=e9eca47507f095471828224b3a2f16f8"
S = "${WORKDIR}"
do_compile () {
    make
}

do_install () {
    install -d ${D}${bindir}/
    install -m 0755 ${S}/route_rule ${D}${bindir}/
}

FILES_${PN} = "${bindir}/route_rule"
