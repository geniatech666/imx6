SUMMARY = "All KF5 packages - just for build test"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

inherit packagegroup

TIER_1 = " \
    attica \
    bluez-qt \
    breeze-icons \
    karchive \
    kcodecs \
    kconfig \
    kcoreaddons \
    kdbusaddons \
    kdnssd \
    kguiaddons \
    kholidays \
    ki18n \
    kidletime \
    kirigami \
    kitemmodels \
    kitemviews \
    kplotting \
    kwayland \
    kwidgetsaddons \
    kwindowsystem \
    modemmanager-qt \
    networkmanager-qt \
    prison \
    solid \
    sonnet \
    syntax-highlighting \
    threadweaver \
"

TIER_2 = " \
    kactivities-stats \
    kauth \
    kcompletion \
    kcrash \
    kdoctools \
    kfilemetadata \
    kimageformats \
    kjobwidgets \
    kpackage \
    kpty \
    kunitconversion \
"

TIER_3 = " \
    baloo \
    kactivities \
    kbookmarks \
    kcmutils \
    kconfigwidgets \
    kdeclarative \
    kded \
    kdesignerplugin \
    kdesu \
    kdewebkit \
    kemoticons \
    kglobalaccel \
    kiconthemes \
    kinit \
    kio \
    knewstuff \
    knotifications \
    knotifyconfig \
    kparts \
    kpeople \
    krunner \
    kservice \
    ktexteditor \
    ktextwidgets \
    kwallet \
    kxmlgui \
    kxmlrpcclient \
    oxygen-icons5 \
    plasma-framework \
    purpose \
    qqc2-desktop-style \
"

TIER_4 = " \
    frameworkintegration \
"

PORTING_AIDS_TIER_1 = " \
    kjs \
"

PORTING_AIDS_TIER_3 = " \
    kdelibs4support \
    khtml \
    kjsembed \
    kross \
"

UNTIER = " \
    extra-cmake-modules \
"

RDEPENDS_${PN} = " \
    ${TIER_1} \
    ${TIER_2} \
    ${TIER_3} \
    ${TIER_4} \
    ${PORTING_AIDS_TIER_1} \
    ${PORTING_AIDS_TIER_3} \
    ${UNTIER} \
"
