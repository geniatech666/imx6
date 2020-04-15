SUMMARY = "LXQt system settings center"
LICENSE = "LGPLv2.1"
LIC_FILES_CHKSUM = "file://LICENSE;md5=435ed639f84d4585d93824e7da3d85da"

inherit lxqt distro_features_check gtk-icon-cache

REQUIRED_DISTRO_FEATURES = "x11"

DEPENDS += "qtsvg liblxqt libkscreen libxcursor"

SRCREV = "9bd6ae9205909f70101aebaa7f316478965905fe"
PV = "0.13.0"

# there are no dev files / pack liblxqt-config-cursor.so correctly
FILES_${PN} += " \
    ${datadir}/icons \
    ${datadir}/lxqt/icons \
    ${libdir}/liblxqt-config-cursor.so \
"
FILES_${PN}-dev = ""
