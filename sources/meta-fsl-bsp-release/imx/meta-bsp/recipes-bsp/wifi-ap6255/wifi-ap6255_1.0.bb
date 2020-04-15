DESCRIPTION = "WIFI ap6255 helper"
SECTION = "wifi-ap6255"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://fw_bcm43455c0_ag.bin;md5=c72a7f4cbab7db26ff8a93e36dbaca66 \
                    file://nvram_ap6255.txt;md5=e9d2af21733fd06de3eb7583cc0b934b"


SRCREV = "${AUTOREV}"
SRC_URI = "file://fw_bcm43455c0_ag.bin \
           file://nvram_ap6255.txt"

S = "${WORKDIR}"

inherit allarch

do_compile() {
}

do_install() {
 install -d ${D}${sysconfdir}/firmware
 install -m 0755 ${WORKDIR}/fw_bcm43455c0_ag.bin ${D}${sysconfdir}/firmware
 install -m 0755 ${WORKDIR}/nvram_ap6255.txt ${D}${sysconfdir}/firmware
}
