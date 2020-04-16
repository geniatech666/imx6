#!/bin/bash
#
# Copyright (c) 2012, The Linux Foundation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

set -o errexit

WORK_DIR=`pwd`
KERNEL_PATH="tmp/work-shared/${BUILD_MACHINE}"
UBOOT_PATH="tmp/work/${BUILD_MACHINE}-poky-linux-gnueabi/u-boot-imx/2018.03-r0"
KERNEL_BUILD_PATH="tmp/work/${BUILD_MACHINE}-poky-linux-gnueabi/linux-imx/4.14.78-r0/build"
BUILD_IMAGE_PATH="${WORK_DIR}/tmp/deploy/images/${BUILD_MACHINE}"
LOOGN_COMMON_PATH="${WORK_DIR}/../loong/common"
LOONG_BOARD_DIR="${WORK_DIR}/../loong/devices/${BUILD_PROJECT}"
OUT_IMAGE_PATH="out"


usage() {
cat <<USAGE

Usage:
    bash $0 <TARGET_PRODUCT> [OPTIONS]

Description:
    Builds yocot for given TARGET_PRODUCT

OPTIONS:
    -c, --clean_build
        Clean build - build from scratch by removing entire out dir

    -h, --help
        Display this help message

    -i, --image
        Specify image to be build/re-build (uboot/bootimg/rootfs)

USAGE
}

dts_deconfig_copy(){
    echo -e "\n dts deconfig copy\n"
    cp -rf -P ../loong/devices/${BUILD_PROJECT}/arch/ ../geniatech/kernel-source/

}

dts_deconfig_reset(){
    echo -e "\n dts deconfig reset\n"
    cd  ../geniatech/kernel-source/
    #reset arm and arm64
    git checkout arch/arm/configs
    git checkout arch/arm/boot/dts
    git checkout arch/arm64/configs
    git checkout arch/arm64/boot/dts
    cd -
}
clean_build() {
    echo -e "\n clean all output files\n"
    bitbake -c clean -v fsl-image-qt5-validation-imx
    bitbake -c cleansstate linux-imx
}

compile_o_file() {
    echo -e "\n compile o file\n"
    cd ../geniatech/kernel-source/
    for i in `find . -type f -name "*.o"` ; do
	cp --parents -rf $i ${BUILDDIR}/${KERNEL_BUILD_PATH}/
    done
    cd -
}

build_bootimg() {
    echo -e "\nINFO: Build bootimage \n"
    bitbake -c cleansstate linux-imx
    bitbake -c patch linux-imx
    #ln the soft link
    kernel_link
    dts_deconfig_copy
    bitbake -c compile -f -v linux-imx
    compile_o_file
    bitbake -c compile -f -v linux-imx
    bitbake linux-imx -c compile_kernelmodules -f -v
    bitbake -c deploy -f -v linux-imx
    dts_deconfig_reset
}

build_uboot() {
    echo -e "\nINFO: Build uboot\n"
    bitbake -c cleansstate u-boot-imx
    bitbake -c patch u-boot-imx
    #ln the soft link
    uboot_link
    bitbake -c compile -f -v u-boot-imx
    bitbake -c deploy -f -v u-boot-imx
}

out_image () {
	if [  ! -d ${OUT_IMAGE_PATH} ] ; then
		mkdir ${OUT_IMAGE_PATH}
	fi  
	rm -rf ${OUT_IMAGE_PATH}/*
	cp -rf ${LOOGN_COMMON_PATH}/uuu.exe ${OUT_IMAGE_PATH}
	cp -rf ${LOONG_BOARD_DIR}/uuu.auto ${OUT_IMAGE_PATH}
	cp  ${BUILD_IMAGE_PATH}/u-boot-${BUILD_MACHINE}.imx  ${OUT_IMAGE_PATH}
	cp  ${BUILD_IMAGE_PATH}/fsl-image-qt5-validation-imx-${BUILD_MACHINE}.sdcard.bz2 ${OUT_IMAGE_PATH}
	

}

build_rootfs() {
    echo -e "\nINFO: Build rootfs\n"
    clean_build
    build_bootimg
    build_uboot
    echo -e "\nINFO: make flash img \n"
    bitbake fsl-image-qt5-validation-imx
    out_image
}

kernel_link(){
        if [  -d ${KERNEL_PATH}/kernel-source ] ; then

                if [ ! -h ${KERNEL_PATH}/kernel-source ] ; then
                cd ${KERNEL_PATH}

                        if [  -d "kernel-source-ori" ] ; then
                        echo -e " rm kernel-source-ori"
                        rm -rf kernel-source-ori
                        fi

                echo -e "build kernel-source soft link"
                mv  kernel-source kernel-source-ori
                ln -s ../../../../geniatech/kernel-source kernel-source
                cd -
                fi

        fi
}

uboot_link(){
        if [  -d ${UBOOT_PATH}/git ] ; then

                if [ ! -h ${UBOOT_PATH}/git ] ; then
                cd ${UBOOT_PATH}

                        if [  -d "git-ori" ] ; then
                        echo -e " rm uboot-source-ori"
                        rm -rf git-ori
                        fi

                echo -e "build uboot-source soft link"
                mv  git git-ori
                ln -s ../../../../../../geniatech/uboot-source git
                cd -
                fi

        fi
}
# Setup getopt.
long_opts="clean_build,help,image:,"
getopt_cmd=$(getopt -o chi: --long "$long_opts" \
            -n $(basename $0) -- "$@") || \
            { echo -e "\nERROR: Getopt failed. Extra args\n"; usage; exit 1;}

eval set -- "$getopt_cmd"

if [ $# -eq 1 ]; then
    echo -e "\n please input params\n"
    usage
    exit 1
fi

if [ $# -eq 2 ]; then
    echo -e "\n please input command\n"
    usage
    exit 1
fi

while true; do
    case "$1" in
        -c|--clean_build) CLEAN_BUILD="true";;
        -h|--help) usage; exit 0;;
        -i|--image) IMAGE="$2"; shift;;
        --) shift; break;;
    esac
    shift
done

if [ "$CLEAN_BUILD" = "true" ]; then
    clean_build
fi

if [ -n "$IMAGE" -a "$BUILD_MACHINE" -a "$BUILD_PROJECT" ]; then
	echo -e "IMAGE:${IMAGE}"
	echo -e "BUILD_MACHINE:${BUILD_MACHINE}"
	echo -e "BUILD_PROJECT:${BUILD_PROJECT}"
	build_$IMAGE
else
	echo -e " IMAGE or BUILD_MACHINE or BUILD_PROJECT not correct"
fi
