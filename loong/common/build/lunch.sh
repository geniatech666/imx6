get_char()
{
	SAVEDSTTY=`stty -g`
	stty -echo
	stty cbreak
	dd if=/dev/tty bs=1 count=1 2> /dev/null
	stty -raw
	stty echo
	stty $SAVEDSTTY
}

support_project_info()
{
	echo  "support project information"
	echo  "--> 1) for xwayland-imx6ull"
	echo  "--> 2) for xwayland-imx6qpsabresd"
}


support_project_info

c=`get_char`
#echo "$c"

case "$c" in
	'1' )
                echo  "select 1 for xwayland-imx6ull"
                sleep 1
                DISTRO=fsl-imx-xwayland MACHINE=imx6ull14x14evk source fsl-setup-release.sh -b build-xwayland-imx6ull14x14evk

	;;
	
        '2' ) 
                echo  "select 2 for xwayland-imx6qpsabresd"
                sleep 1
                DISTRO=fsl-imx-xwayland MACHINE=imx6qpsabresd source fsl-setup-release.sh -b build-xwayland-imx6qpsabresd
        ;;


	* ) 
		echo  "no project select"	
	;;
esac


