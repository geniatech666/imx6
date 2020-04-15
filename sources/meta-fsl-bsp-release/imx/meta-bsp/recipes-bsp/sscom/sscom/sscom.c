#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

static int set_opt(int fd,int nSpeed,int nBits,char nEvent,int nStop)
{
    	struct termios newttys1,oldttys1;

    	if(tcgetattr(fd,&oldttys1)!=0)         //保存原先串口配置
    	{
        	perror("Setupserial 1");
        	return -1;
    	}

	bzero(&newttys1,sizeof(newttys1));       //将一段内存区域的内容全清为零
	newttys1.c_cflag|=(CLOCAL|CREAD );       //CREAD 开启串行数据接收，CLOCAL并打开本地连接模式   

	newttys1.c_cflag &=~CSIZE;              //设置数据位数
	switch(nBits)     //选择数据位  
 	{
        	case 7:
                	newttys1.c_cflag |=CS7;
                	break;
        	case 8:
                	newttys1.c_cflag |=CS8;
                	break;
 	}

	switch( nEvent )    //设置校验位  
 	{	
   	case '0':       //奇校验  
           	newttys1.c_cflag |= PARENB;             //开启奇偶校验  
           	newttys1.c_iflag |= (INPCK | ISTRIP);   //INPCK打开输入奇偶校验；ISTRIP去除字符的第八个比特  
           	newttys1.c_cflag |= PARODD;             //启用奇校验(默认为偶校验)  
           	break;
  	case 'E' :       //偶校验  
           	newttys1.c_cflag |= PARENB;             //开启奇偶校验  
           	newttys1.c_iflag |= ( INPCK | ISTRIP);  //打开输入奇偶校验并去除字符第八个比特  
           	newttys1.c_cflag &= ~PARODD;            //启用偶校验；  
           	break;
  	case 'N':     //关闭奇偶校验
           	newttys1.c_cflag &= ~PARENB;
           	break;
   	}

     	switch( nSpeed )        //设置波特率  
     	{
        case 2400:
                 cfsetispeed(&newttys1, B2400);           //设置输入速度
                 cfsetospeed(&newttys1, B2400);           //设置输出速度
                 break;
        case 4800:
                 cfsetispeed(&newttys1, B4800);
                 cfsetospeed(&newttys1, B4800);
                 break;
        case 9600:
                 cfsetispeed(&newttys1, B9600);
                 cfsetospeed(&newttys1, B9600);
                 break;
        case 115200:
                 cfsetispeed(&newttys1, B115200);
                 cfsetospeed(&newttys1, B115200);
                 break;
        default:
                 cfsetispeed(&newttys1, B9600);
                 cfsetospeed(&newttys1, B9600);
                 break;
     }

     if( nStop == 1)                      //设置停止位；若停止位为1，则清除CSTOPB，若停止位为2，则激活CSTOPB。  
     {
        newttys1.c_cflag &= ~CSTOPB;      //默认为送一位停止位；  
     }
     else if( nStop == 2)
     {
        newttys1.c_cflag |= CSTOPB;       //CSTOPB表示送两位停止位；  
     }

     //设置最少字符和等待时间，对于接收字符和等待时间没有特别的要求时
     newttys1.c_cc[VTIME] = 0;        //非规范模式读取时的超时时间；  
     newttys1.c_cc[VMIN]  = 0;        //非规范模式读取时的最小字符数；  

     tcflush(fd ,TCIFLUSH);           //tcflush清空终端未完成的输入/输出请求及数据；TCIFLUSH表示清空正收到的数据，且不读取出来 

     // 在完成配置后，需要激活配置使其生效
     if((tcsetattr( fd, TCSANOW,&newttys1))!=0) //TCSANOW不等数据传输完毕就立即改变属性  
     {
         perror("com set error");
         return -1;
     }
    return 0;
} /* ----- End of if()  ----- */
static int call_number (int fd)
{
    getchar();                             //将缓冲区回车吃掉

    int count=0;
    char call[20]="atd";     
    char number[20];
    char reply[128];

    printf("enter you call number\n");
    if(NULL==fgets(number,20,stdin))      //输入电话号码，其中fgets在读入一个字符串后在字符串尾端默认加入\n字符
        exit(0);           //这个语句的功能可以用gets实现，区别在于 fgets 读入的含 "\n"（最后一个字符），gets 不含 "\n"

    while(strlen(number)!=12)           
    {
        printf("please again number\n");
        if(NULL==fgets(number,20,stdin))
            exit(0);
        if(count==3)
            exit(0);
        count++;
    }

    number[strlen(number)-1]='\0'; //将刚才fgets读入的字符串尾端去除\n字符
    strcat(call,number);     
    strcat(call,";\r");            // \r是换行字符

    write(fd,call,strlen(call));   //向串口拨打号码
    printf("write %s\n",call);
    sleep(3);  
    memset(reply,0,sizeof(reply));
    read(fd,reply,sizeof(reply));
    printf("%s\n",reply);

    printf("=================================================\n");
    printf("number is calling,please press 'a' hang up \n");
    printf("=================================================\n");

   while('a'!=getchar())
        printf("please again input 'a' to hung up \n");

    memset(call,0,sizeof(call));
    strcpy(call,"ATH\r"); 
    write(fd,call,strlen(call));
    sleep(1);
    memset(reply,0,sizeof(reply));
    read(fd,reply,sizeof(reply));
    printf("%s\n",reply);
    printf("has hung up\n");


    return 0;
} /* ----- End of call_number()  ----- */

static int send_message (int fd)
{
    int count=0;
    char  cmgf[]="at+cmgf=1\r";  //设置短信发送模式为text （0-PDU;1-text）
    char  cmgs[128]="at+cmgs=\"";
    char  send_number[16];
    char  message[128];
    char  reply[128];

    getchar();                   //吃掉缓冲区回车
    printf("enter send_message number :\n");
    if(NULL==fgets(send_number,16,stdin))
        exit(0);
    while(12!=strlen(send_number))
    {
        getchar();
        printf("please again input number\n");
        if(NULL==fgets(send_number,16,stdin))
             exit(0);
        if(count==3)
            exit(0);
        count++;
    }

    send_number[strlen(send_number)-1]='\0';   //去除字符串末端读入的换行符\n;
    strcat(cmgs,send_number);
    strcat(cmgs,"\"\r");

    printf("enter send_message :\n");
    if(NULL==fgets(message,128,stdin))
        exit(0);

    message[strlen(message)-1]='\0';
    strcat(message,"\x1a");

    /* write cmgf */

    write(fd,cmgf,strlen(cmgf));
    printf("write %s\n",cmgf);
    sleep(2);
    memset(reply,0,sizeof(reply));
    read(fd,reply,sizeof(reply));
    printf("%s\n",reply);

    /* write cmgs */

    write(fd,cmgs,strlen(cmgs));
    printf("writr %s\n",cmgs);
    sleep(5);
    //memset(reply,0,sizeof(reply));
    //read(fd,reply,sizeof(reply));
    //printf("%s\n",reply);

    /*write  message*/

    write(fd,message,strlen(message));
    printf("writr %s\n",message);
    sleep(4);
    memset(reply,0,sizeof(reply));
    read(fd,reply,sizeof(reply));
    printf("%s\n",reply);


    return 0;
} /* ----- End of send_message()  ----- */

static int check_apn(int fd)
{
	char  cmgf[]="AT+COPS?\r\n"; //select the simcard which one		
    	char  reply[128];	
	char mobil[]="004300300034";//AT reply message
	char mobil2[]="MOBILE";
	char unicom[]="0043003000";
	char unicom2[]="UNICOM";	
	long int reply_count =0;
        //FILE *fp = fopen("/root/apn.txt","w+");
	int r=0,w=0,j=0;
	w = write(fd,cmgf,strlen(cmgf));
	printf("write:%s :r:%d\n",cmgf,w);
	memset(reply,0,sizeof(reply));
    	while(1)
	{
		r = read(fd,reply,sizeof(reply));
    		printf("reply:%s:result:%d\n",reply,r);
		//fwrite(reply,1,sizeof(reply),fp);
		if(strstr(reply,mobil) || strstr(reply,mobil2))
		{
			//printf("your sim card apn is CMNET\n");
			system("cp /etc/wvdial_cmnet.conf /etc/wvdial.conf");
			break;
		}else if(strstr(reply,unicom) || strstr(reply,unicom2))
		{
			//printf("your sim card apn is 3GNET\n");
			system("cp /etc/wvdial_3gnet.conf /etc/wvdial.conf");
			break;
		}
		if(reply_count <100000)	
		{
			usleep(1000);
			reply_count++;
		}else
		{
			printf("your sim card can't be recognized\n");
			break;
		}
	}	
	return 0;
}

int main (int argc, char **argv)
{
    int fd;
    char select;

    fd = open( "/dev/ttyUSB1", O_RDWR|O_NOCTTY|O_NDELAY);
    if(fd<0){
        perror("Can't Open Serial Port");
        return -1;
    }
     //配置串口
    set_opt( fd,115200,8,'N',1);
    check_apn(fd);

#if 0
    
    /*save that function support send message and call number*/
    printf("==========================================\n");
    printf("gprs call number and send message\n");
    printf("==========================================\n");
    printf("enter your select:  's' is send message,  'c' is call number,   'q' is exit \n");
    select=getchar();
    switch(select)
    {
	case 's':
   		 printf("please enter your target number\n");
		 send_message(fd);
                 break;
	case 'c':
		 printf("please enter the phone you dialed\n");
		 call_number(fd);
                 break;
	case 'a':
		 printf("checking apn!!!!\n");
		 check_apn(fd);
		 break;
        default:
                 check_apn(fd);
		 break;
    }
#endif
    
   close(fd);
    return 0;
} /* ----- End of main() ----- */

