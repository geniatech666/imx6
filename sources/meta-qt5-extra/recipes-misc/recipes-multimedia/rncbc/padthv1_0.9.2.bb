SUMMARY = "An old-school polyphonic additive synthesizer"
HOMEPAGE = "http://padthv1.sourceforge.net/"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"

DEPENDS += " \
    qtbase-native \
    qtbase \
    jack \
    lv2 \
    liblo \
    fftw \
"

inherit qmake5_base autotools-brokensep pkgconfig gtk-icon-cache mime

# fftwf is neon accelerated -> force SINGLE precision
FFTWSINGLEPATCH = "${@bb.utils.contains('TUNE_FEATURES', 'neon', 'file://0001-Build-against-fftw3f-that-supports-NEON-on-ARM.patch', '', d)}"

SRC_URI = " \
    ${SOURCEFORGE_MIRROR}/project/${BPN}/${BPN}/${PV}/${BPN}-${PV}.tar.gz \
    http://linuxsynths.com/Padthv1PatchesDemos/Padthv1Patches.tar.gz;name=linuxsynths-padthv1-presets;subdir=linuxsynths-padthv1-presets \
    file://0001-find-native-qt-build-tools-by-configure-options-auto.patch \
    \
    ${FFTWSINGLEPATCH} \
    file://padthv1.conf \
"
SRC_URI[md5sum] = "af029d5bc08ec93df2bf3827deb7ab92"
SRC_URI[sha256sum] = "b487f2e16f06c0146b2c4fd61f5e5c41d9d88afc3c434e0b0794365c28a08eaa"

SRC_URI[linuxsynths-padthv1-presets.md5sum] = "951484ad2fe404d233a704d444147827"
SRC_URI[linuxsynths-padthv1-presets.sha256sum] = "ad9eadc707784b6931955b1fc63308b9e5dc59d24903e6405e9d34d30794fd0b"

EXTRA_OECONF = " \
    --with-qmake=${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}/qmake \
    --with-moc=${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}/moc \
    --with-uic=${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}/uic \
    --with-lupdate=${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}/lupdate \
    --with-lrelease=${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}/lrelease \
"

do_install_append() {
    install -d ${D}/${datadir}/${BPN}
    install -d ${D}/${datadir}/${BPN}/presets
    cp ${WORKDIR}/linuxsynths-padthv1-presets/*.padthv1 ${D}/${datadir}/${BPN}/presets

    install -d ${D}/${sysconfdir}/skel/.config/rncbc.org
    install -m 0644 ${WORKDIR}/${BPN}.conf ${D}/${sysconfdir}/skel/.config/rncbc.org/
}

PACKAGES =+ "${PN}-presets"

FILES_${PN} += " \
    ${datadir}/appdata \
    ${datadir}/mime \
    ${datadir}/metainfo \
    ${datadir}/icons \
    ${libdir}/lv2 \
"

FILES_${PN}-presets += " \
    ${sysconfdir}/skel/.config/rncbc.org \
    ${datadir}/${BPN}/presets/ \
"
