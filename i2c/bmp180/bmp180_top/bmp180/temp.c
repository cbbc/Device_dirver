#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/*
 * This function is show wait process for transfering
 */
void show_print(int percent,int barlen,int ture)
{
	int i;
	int m,n;
	m = ture % 10;
	n = ture / 10;
	percent = m * 2 + n * 2;
	putchar('[');
	for(i=1;i<= barlen;++i)
		putchar(i*100 <= percent * barlen ? '>' : ' ');
	putchar(']');
	printf("%3d.%dC",n,m);
	for(i=0;i != barlen + 12;++i)
		putchar('\b');
}
	
int main()
{
	int i;
	int data;
	int fd;
	int tmp;

	fd = open("/dev/bmp180",O_RDWR);
	while(1)
	{
		read(fd,(char *)&data,1);
		tmp = data;
		data /= 10;
		show_print(data,50,tmp);
		fflush(stdout);
		sleep(1);
	}
	close(fd);
	return 0;
}
