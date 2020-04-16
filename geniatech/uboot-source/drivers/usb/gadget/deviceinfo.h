
#define DEV_MAGIC  "GENIATECH!"
#define MAGIC_SIZE 20
#define SN_LEN 50
#define MAC_LEN 50
#define PARAMS_PATH "/dev/block/bootdevice/by-name/params"
//for nxp imx6
#define PARAMS_PTN "mmcsdc2"
//#define PARAMS_PTN "params"
#define FLASH_LOCK_MAGIC   0x20181012
struct dev_info
{
	char magic[MAGIC_SIZE];
	char  board_sn[SN_LEN];
	char  mac_addr[MAC_LEN];
	unsigned int flash_lock;
};
