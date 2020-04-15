DESCRIPTION = "Burn key for sn&mac"
SECTION = "libs"
LICENSE = "MIT"
PV = "3"
PR = "r0"

SRC_URI = " \
          file://burn_key \
          "
TARGET_CC_ARCH += "${LDFLAGS}"

INHIBIT_PACKAGE_DEBUG_SPLIT="1"

LIC_FILES_CHKSUM = "file://burn_key;md5=3d6f43389a695832c814435e18952ed9"
S = "${WORKDIR}"
do_compile () {
}

do_install () {
    install -d ${D}${bindir}/
    install -m 0755 ${S}/burn_key ${D}${bindir}/
}

FILES_${PN} = "${bindir}/burn_key"
