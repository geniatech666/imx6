require ${BPN}.inc

DEPENDS += " \
    ${BPN}-native \
    kiconthemes \
"

SRC_URI += "file://0003-Find-native-qrcAlias.patch"

FILES_${PN} += "${datadir}/icons"

PACKAGES =+ "${PN}-binres"
FILES_${PN}-binres = " \
    ${datadir}/icons/breeze/breeze-icons.rcc \
    ${datadir}/icons/breeze-dark/breeze-icons-dark.rcc \
"
