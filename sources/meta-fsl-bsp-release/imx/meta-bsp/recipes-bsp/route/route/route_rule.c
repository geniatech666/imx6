#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#define PATH "/proc/net/route"//系统网关保存地址

struct r_entry {
    int dest;
    int mask;
    int gateway;
    char iface[10];
    struct r_entry *next;
};

struct r_entry *get_route_entry (void)//获取ppp0的默认网关
{
    FILE *fp = NULL;
    fp = fopen (PATH, "r");
    if (NULL == fp) {
        perror ("can't open file!\n");
        exit (EXIT_FAILURE);
    }
 
    char buf[BUFSIZ] = {'\0'};
    int firsttime = 1;
    int firstline = 1;
    int nouse;
    struct r_entry *head, *q, *p;
    head = q = p = NULL;
 
    while (!feof (fp)) {
        fgets (buf, BUFSIZ, fp);
        if (firstline) {
            firstline = 0;
            continue;
        }
 
        p = malloc (sizeof (struct r_entry));
        if (NULL == p) {
            fprintf (stderr, "can not malloc !");
            exit (EXIT_FAILURE);
        }
        sscanf (buf, "%s%x%x%x%x%x%x%x", p->iface, &p->dest, &p->gateway, 
                &nouse, &nouse, &nouse, &nouse, &p->mask);
        p->next = NULL;
 
        /* add to link */
        if (firsttime) {
            head = p;
            q = p;
            firsttime = 0;
        }else {
            q->next  = p;
            q = q->next;
        }
    }
 
    fclose (fp);
    
    return head;
}
 
int del_route_entry (struct r_entry *head)//删除相关脚本默认语言
{
    if (head == NULL) 
        return 0;
 
    struct r_entry *q, *p;
    p = q =head;
    for (p = q = head; p != NULL; p = q) {
        q = q->next;
        free (p);
    }
 
    return 0;
}
 
/*
** display route table
*/
int display_route (void)
{
    struct r_entry *head, *p;
    char buf1[20] = {'\0'};
    char buf2[20] = {'\0'};
    char buf3[20] = {'\0'};
    char ppp0[]="ppp0";
    char buf4[30];
    char route[80]= "sudo route add default gw ";
    char net[]=" dev ppp0 metric 900";
    FILE *fp;
    int i,k=0,j=0;
    fp = fopen("/etc/route.sh","w+");
    head =  get_route_entry ();
    if (head == NULL) {
        fprintf (stderr, "can not get route table\n");
    }
 
    printf ("Destination\tGenmask\t\tGateway\t\tUse Iface\n");
    struct in_addr gateway;
    struct in_addr netmask;
    struct in_addr destion;
    memset (&gateway, 0, sizeof (struct in_addr));
    memset (&netmask, 0, sizeof (struct in_addr));
    memset (&destion, 0, sizeof (struct in_addr));
    for (p = head; p!= NULL; p = p->next) {
        gateway.s_addr = p->gateway;
        netmask.s_addr = p->mask;
        destion.s_addr = p->dest;
        
	if(strstr(p->iface,ppp0))
	{
	
		printf ("%s\t%s\t%s\t%s\n",
                inet_ntop (AF_INET, &destion, buf1, 20),
                inet_ntop (AF_INET, &netmask, buf2, 20),
                inet_ntop (AF_INET, &gateway, buf3, 20),
                p->iface);
		
		for(i=0;i <= sizeof(buf1)-1;i++)
		{
			if(buf1[i] == '0')
			k++;	
		}//if you find another reason that can't meet the ask
		
		if(k == 4)//全是0的ppp0网络节点不符合要求
		{
			printf("there:%s\n",route);
		}else
		{
			printf("buf1:%s:k:%d:buf4:%s\n",buf1,k,buf4);
			
			strcat(route,buf1);
			printf("route:%s\n",route);
			strcat(route,net);
			printf("net:%s\n",route);
			fwrite(route,sizeof(route),1,fp);
			break;
		}
	}	
    }
    
    del_route_entry (head);
    
    return 0;
}
 
int main (int argc, char **argv)
{
    display_route ();
    return 0;
}
