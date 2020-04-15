DESCRIPTION = "suppot  wifi bt module"
SECTION = "wifi-bt"
LICENSE = "MIT"
PV = "3"
PR = "r0"

TARGET_CC_ARCH += "${LDFLAGS}"
INSANE_SKIP_${PN} = "ldflags"
INSANE_SKIP_${PN}-dev = "ldflags"

SRC_URI = " \
          file://rk_wifi_init \
	  file://brcm_patchram_plus1 \
	  file://fw_bcm43455c0_ag.bin \
	  file://nvram_ap6255.txt \
	  file://BCM4345C0.hcd \
          "

LIC_FILES_CHKSUM = "file://fw_bcm43455c0_ag.bin;md5=c72a7f4cbab7db26ff8a93e36dbaca66 \
		   "

S = "${WORKDIR}"

do_compile () {
}

do_install () {
    install -d ${D}${bindir}/
    install -m 0755 ${S}/rk_wifi_init ${D}${bindir}/
    install -m 0755 ${S}/brcm_patchram_plus1 ${D}${bindir}/

    install -d ${D}${sysconfdir}/firmware
    install -m 0755 ${WORKDIR}/fw_bcm43455c0_ag.bin ${D}${sysconfdir}/firmware
    install -m 0755 ${WORKDIR}/nvram_ap6255.txt ${D}${sysconfdir}/firmware
    install -m 0755 ${WORKDIR}/BCM4345C0.hcd ${D}${sysconfdir}/firmware
}

#FILES_${PN} = "${bindir}/rk_wifi_init"
