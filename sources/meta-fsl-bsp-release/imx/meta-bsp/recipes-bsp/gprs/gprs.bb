DESCRIPTION = "gprs"
SECTION = "libs"
LICENSE = "MIT"
PV = "3"
PR = "r0"
SRCREV = "${AUTOREV}"

SRC_URI = " \
          file://wvdial.conf \  
	  file://gprs_call.sh"

TARGET_CC_ARCH += "${LDFLAGS}"

#LIC_FILES_CHKSUM = "file://wvdial.conf;md5=5e54b4bf220d782c9d4c8b408aa91ef8 \
#		    file://gprs_all.sh;md5=5e54b4bf220d782c9d4b408aa91ef8"
LIC_FILES_CHKSUM = "file://wvdial.conf;md5=5e54b4bf220d782c9d4c8b408aa91ef8"

S = "${WORKDIR}"

inherit allarch

do_compile () {
}

do_install () {
    install -d ${D}${sysconfdir}
    install -d ${D}${sysconfdir}
    
    install -m 0755 ${WORKDIR}/gprs_call.sh ${D}/${sysconfdir}/
    install -m 0755 ${WORKDIR}/wvdial.conf ${D}/${sysconfdir}/
}
