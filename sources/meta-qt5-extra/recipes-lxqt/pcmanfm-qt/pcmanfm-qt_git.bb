SUMMARY = "File manager and desktop icon manager (Qt port of PCManFM and libfm)"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=4641e94ec96f98fabc56ff9cc48be14b"

REQUIRED_DISTRO_FEATURES = "x11"

inherit lxqt pkgconfig distro_features_check

DEPENDS += "libfm-qt"

SRCREV = "5ada22db99d3acd05c250fe3409dac8ecf7a3380"
PV = "0.13.0"

RRECOMMENDS_${PN} = "gvfs gvfsd-trash eject"
