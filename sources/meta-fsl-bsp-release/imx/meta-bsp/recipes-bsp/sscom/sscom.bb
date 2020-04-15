DESCRIPTION = "sscom"
SECTION = "libs"
LICENSE = "MIT"
PV = "3"
PR = "r0"

SRC_URI = " \
          file://sscom.c \
          file://makefile \
          "
TARGET_CC_ARCH += "${LDFLAGS}"

LIC_FILES_CHKSUM = "file://sscom.c;md5=fea092e87372e82fe82987edadb01e0e"
S = "${WORKDIR}"
do_compile () {
    make
}

do_install () {
    install -d ${D}${bindir}/
    install -m 0755 ${S}/sscom ${D}${bindir}/
}

FILES_${PN} = "${bindir}/sscom"
