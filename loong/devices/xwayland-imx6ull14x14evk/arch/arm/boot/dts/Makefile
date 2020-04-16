# SPDX-License-Identifier: GPL-2.0
ifeq ($(CONFIG_OF),y)
dtb-$(CONFIG_SOC_IMX6ULL) += \
	imx6ull-14x14-evk.dtb \
	imx6ull-14x14-evk-btwifi.dtb \
	imx6ull-14x14-evk-btwifi-oob.dtb \
	imx6ull-14x14-evk-emmc.dtb \
    imx6ull-14x14-evk-usb-certi.dtb \
	imx6ull-14x14-evk-gpmi-weim.dtb
endif

dtstree		:= $(srctree)/$(src)
dtb-$(CONFIG_OF_ALL_DTBS) := $(patsubst $(dtstree)/%.dts,%.dtb, $(wildcard $(dtstree)/*.dts))

always		:= $(dtb-y)
clean-files	:= *.dtb
