# Append to remove /proc/cpu/aligntment from alignmnet script
FILESEXTRAPATHS_prepend := "${THISDIR}/arm:"

SRC_URI_append_imx = " file://alignment.sh "
SRC_URI_append_imx += " file://bcmdhd-wift-bt-boot.sh "
SRC_URI_append_imx += " file://rtc-sync.sh "

PACKAGE_ARCH = "${MACHINE_ARCH}"

do_install_append() {
    install -m 0755 ${WORKDIR}/bcmdhd-wift-bt-boot.sh ${D}${sysconfdir}/init.d
    update-rc.d -r ${D} bcmdhd-wift-bt-boot.sh start 99 2 3 4 5 .

    install -m 0755 ${WORKDIR}/rtc-sync.sh ${D}${sysconfdir}/init.d
    update-rc.d -r ${D} rtc-sync.sh start 99 2 3 4 5 .
}
