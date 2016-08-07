#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct input_event ievent;
	int fd = open("/dev/input/event1",O_RDWR);
	if(fd < 0)
	{
		printf("Unable open /dev/input/event0\n");
		return -1;
	}
	printf("Succeed to open device\n");
	while(1)
	{
		if(read(fd,&ievent,sizeof(struct input_event)) < 0)
		{
			printf("Unable read data from event\n");
			return -1;
		}
		printf("!");
		if(ievent.type == EV_KEY)
		{
			printf("The ievent.code [%d]\n",ievent.code);
		}
		printf(".");
	}
	close(fd);
	return 0;
}
