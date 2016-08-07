
#include<unistd.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<stdio.h>
#include<fcntl.h>
#include<assert.h>
#include <linux/soundcard.h>

#define WAV_FILE "test.wav"
#define DEV_DSP "/dev/dsp"
#define DEV_MIXER "/dev/mixer"
#define SampleRate 11025
#define Channels 1
#define Bits   16
#define BufTime 3
#define BUFFER_SIZE (SampleRate * Channels * Bits * BufTime) / 8 

int main(void) 
{
	int fd_file, fd_dsp, fd_mixer;
	int ret, arg;
	int vol = 100;
	char tmp[BUFFER_SIZE] ;
	fd_file = open (WAV_FILE, O_RDONLY);
	fd_dsp = open (DEV_DSP, O_RDWR);
	if (fd_file < 0 || fd_dsp < 0) {
		printf("No such file or device.\n");
	}

	fd_mixer = open (DEV_MIXER, O_RDONLY);
	ioctl(fd_mixer, MIXER_WRITE(SOUND_MIXER_VOLUME), &vol); 
	arg = SampleRate;
	ioctl(fd_dsp, SOUND_PCM_WRITE_RATE, &arg);
	arg = Channels;
	ioctl(fd_dsp, SOUND_PCM_WRITE_CHANNELS, &arg);
	arg = Bits;
	ioctl(fd_dsp, SOUND_PCM_WRITE_BITS, &arg);
	
	do { 
		ret = read(fd_file, tmp , BUFFER_SIZE);
		write (fd_dsp, tmp, ret);
	} while (ret == BUFFER_SIZE);
	
	free(tmp);
	close(fd_file);
	close(fd_dsp);
}

