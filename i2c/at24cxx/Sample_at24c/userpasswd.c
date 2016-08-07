#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>


#define DEV_NAME "/dev/at24c"

int main()
{
	int ret;
	int fd;
	char username[16];
	char passwd[16];
	char u_name[16];
	char p_name[16];
	int u_len,p_len;
	/*
	 * User operation
	 */
	printf("=====Welcome to Buddy safe.Inc=====\n");
retry:
	printf("==Please input your name:\n>");
	scanf("%s",username);
	u_len = strlen(username);
	printf("==Please input your passwd:\n>");
	scanf("%s",passwd);
	p_len = strlen(passwd);
	/*
	 * open device
	 */
	fd = open(DEV_NAME,O_RDWR);
	if(fd < 0)
		exit(1);
	/*
	 * read username from at24c
	 */
	lseek(fd,0,SEEK_SET);
	memset(u_name,0,16);
	read(fd,u_name,u_len);
	u_name[u_len] = '\0';
	if(strncmp(username,u_name,u_len) != 0)
	{
		printf("Username error.Retry!\n");
		close(fd);
		goto retry;
	}
	lseek(fd,0x10,SEEK_SET);
	memset(p_name,0,16);
	read(fd,p_name,p_len);
	passwd[p_len] = '\0';
	if(strncmp(passwd,p_name,p_len) != 0)
	{
		printf("Password error.Retry!\n");
		close(fd);
		goto retry;
	}
	printf("==Succeed login.\n");
	close(fd);
}
