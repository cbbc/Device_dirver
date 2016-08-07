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

#define DEV_NAME "/dev/bmp180"

int main()
{
	int ret;
	int fd;
	char *tmp;
	int i;
	
	/*
	 * open device
	 */
	fd = open(DEV_NAME,O_RDWR);
	if(fd < 0)
	{
		printf("Unable to open device\n");
		exit(1);
	}
	printf("Succeed to open device\n");
	/*
	 * read data
	 */
	ret = read(fd,tmp,2);
	if(ret <= 0)
		printf("Fail to read data\n");
	printf("Succeed to read data is %s\n",tmp);

	close(fd);
	return 0;
}
