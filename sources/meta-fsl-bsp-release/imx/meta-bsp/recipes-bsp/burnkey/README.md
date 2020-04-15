burnkey
============

burnkey for sn&mac

##Intructions:
1. Clone this repo to the meta-XXX/recipe-YYY/
2. In yocto `build` diretory, source the environment `source poky/oe-XXXXX`
3. Using the `bitbake -s | grep burnkey` to see and validate the pacakge version
4. bitbake the bb file to build the manumaltest: `bitbake -b <path to the>manualtest.bb -v`
    or just using the `bitake burnkey` to build the target

