#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/soundcard.h>


#define LENGTH 3         /* set store time */
#define RATE   8000      /* set rate */
#define SIZE   8         /* set size */
#define CHS    1         /* channel number */
/*
 * buffer
 */
unsigned char buf[ LENGTH * RATE * SIZE * CHS / 8 ]; 


int main()
{
	int fd;
	int arg;
	int state;

	/*
	 * open dev
	 */
	fd = open("/dev/dsp",O_RDWR);
	if(fd < 0)
	{
		printf("Unable to open file\n");
		exit(1);
	}
	/*
	 * set sample bits
	 */
	arg = SIZE;
	state = ioctl(fd,SOUND_PCM_WRITE_BITS,&arg);
	/*
	 * set sample channel
	 */
	arg = CHS;
	state = ioctl(fd,SOUND_PCM_WRITE_RATE,&arg);
	/*
	 * loop until push Ctr-C
	 */
	while(1)
	{
		printf("Say something:\n");
		state = read(fd,buf,sizeof(buf));/* record */
		if(state == 0)
			printf("Unable to read\n");

		printf("You said:\n");
		state = write(fd,buf,sizeof(buf));/* play sound */
		if(state == 0)
			printf("failt to write\n");

		state = ioctl(fd,SOUND_PCM_SYNC,0);
	}
}
