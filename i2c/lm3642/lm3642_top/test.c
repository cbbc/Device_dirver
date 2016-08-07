#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int fd;
	char tmp[20];
	if((fd = open("/dev/lm3642",O_RDWR)) == 0)
	{
		printf("open failed\n");
		return -1;
	}
	printf("open device\n");

	while(1)
		read(fd,tmp,2);
	close(fd);
	return 0;
}
