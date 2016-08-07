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
	printf("=====Welcome to register Buddy safe.Inc=====\n");
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
	write(fd,username,u_len);
	lseek(fd,16,SEEK_SET);
	write(fd,passwd,p_len);
	printf("==Succeed register.\n");
	close(fd);
}
