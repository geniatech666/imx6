SUMMARY = "Audio plugin host"
HOMEPAGE = "http://kxstudio.linuxaudio.org/Applications:Carla"
LICENSE = "GPLv2 & LGPLv3"
LIC_FILES_CHKSUM = " \
    file://doc/GPL.txt;md5=4641e94ec96f98fabc56ff9cc48be14b \
    file://doc/LGPL.txt;md5=e6a600fd5e1d9cbde2d983680233ad02 \
"

SRC_URI = " \
    git://github.com/falkTX/Carla.git \
    file://0001-do-not-try-to-cross-run-carla-lv2-export.patch \
    file://0002-Do-not-try-to-find-Qt5-host-bins-it-won-t-work.patch \
"
SRCREV = "430740a896d8f7546e02b524302e2cfda4509ff9"
S = "${WORKDIR}/git"
PV = "1.9.11+git${SRCPV}"

REQUIRED_DISTRO_FEATURES = "x11"

inherit qmake5_base autotools-brokensep pkgconfig qemu-ext distro_features_check mime gtk-icon-cache

DEPENDS += " \
    python3-pyqt5-native \
    qtbase-native \
    qtbase \
    gtk+ \
    gtk+3 \
    liblo \
    pulseaudio \
    fluidsynth \
    libsndfile1 \
"

EXTRA_OEMAKE += " \
    DEFAULT_QT=5 \
    PREFIX=${prefix} \
    NOOPT=true \
    HAVE_PYQT=true \
    HAVE_PYQT4=false \
    HAVE_PYQT5=true \
    SKIP_STRIPPING=true \
"

export QT5_HOSTBINS="${OE_QMAKE_PATH_EXTERNAL_HOST_BINS}"

do_configure() {
    oe_runmake features
}

do_compile_append() {
    cd ${S}/bin
    ${@qemu_run_binary_local(d, '${STAGING_DIR_TARGET}', 'carla-lv2-export')}
}

FILES_${PN} += " \
    ${datadir}/icons \
    ${datadir}/mime \
    ${libdir}/lv2 \
    ${libdir}/vst \
"

INSANE_SKIP_${PN} = "dev-so"

RDEPENDS_${PN} += "python3-pyqt5 bash"
