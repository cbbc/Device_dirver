/* 
 * File:   ffaudio.c
 * Author: root
 *
 * Created on 2008年2月25日, 下午1:00
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#define SOUND_DEV "/dev/dsp"
/*
 * 
 */
enum MODE{    
    PLAY = 0x00,
    REC  = 0x01,
    UNKOWN=0x80
};

unsigned int channels;
unsigned int speed;
unsigned int format;
unsigned char filename[80];
enum MODE tmode;
int omode;

void usage(char *argv){
    fprintf(stderr,"usage: %s [option] filename\n",argv);
    fprintf(stderr,"option:");
    fprintf(stderr,"\tplay/rec play or recode\n");
    fprintf(stderr,"\t-c channel\n");
    fprintf(stderr,"\t-s speed\n");
    fprintf(stderr,"\t-f sample bits\n");
    exit(0);
}

int analyze_parament(int argc,char **argv){
    int i;
    if(argc==1)usage(argv[0]);
    argc--;
    i=1;
    tmode=UNKOWN;
    channels=1;
    speed=44100;
    format=16;
    while(i<argc){
        if(!strcmp(argv[i],"play")){
            tmode=PLAY;
            omode=O_WRONLY;
        }else if(!strcmp(argv[i],"rec")){
            tmode=REC;
            omode=O_RDONLY;
        }else if(!strcmp(argv[i],"-c")){
            if(!argv[++i])usage(argv[0]);
            channels=atoi(argv[i]);
            //fprintf(stderr,"channel=%d\n",channel);
        }else if(!strcmp(argv[i],"-s")){
            if(!argv[++i])usage(argv[0]);
            speed=atoi(argv[i]);
            //fprintf(stderr,"speed=%d\n",speed);
        }else if(!strcmp(argv[i],"-f")){
            if(!argv[++i])usage(argv[0]);
            format=atoi(argv[i]);
            //fprintf(stderr,"bits=%d\n",bits);
        }else{
            fprintf(stderr,"Unkonw option [%s]\n",argv[i]);
            usage(argv[0]);
        }
        i++;
    }
    if(tmode==UNKOWN){
        fprintf(stderr,"You must specify a mode [play/rec].\n");
        usage(argv[0]);
    }
    if(argv[i])strcpy(filename,argv[i]);
    return 1;
}

int set_pcm_parm(int fd){
    int tformat;
    if(format==8){
        tformat=AFMT_U8;
    }else if(format==16){
        tformat=AFMT_S16_LE;
    }else{
        format=AFMT_S16_LE;
    }
    if (ioctl(fd, SNDCTL_DSP_SETFMT, &tformat) == -1) {        
        perror("SNDCTL_DSP_SETFMT");
        exit(1);
        
    }

    if (ioctl(fd, SNDCTL_DSP_CHANNELS, &channels) == -1) {
        perror("SNDCTL_DSP_CHANNELS");
        exit(1);
    }
    if (ioctl(fd, SNDCTL_DSP_SPEED, &speed) == -1) {
        perror("SNDCTL_DSP_SPEED");
        exit(1);
    } else
        printf("Support 44.1 KHz , Actual Speed : %d \n", speed);
}

void start_play(int audio_fd){
    int count;
    int music_fd;
    signed short applicbuf[2048];
    if ((music_fd = open(filename, O_RDONLY, 0)) == -1) {
        perror(filename);
        exit(1);
    }
    while ((count = read(music_fd, applicbuf, 2048)) > 0) {
        write(audio_fd, applicbuf, count);
        
    }
    close(music_fd);
}

void start_recode(int audio_fd){
    int totalbyte= speed * channels * 2 * 60 * 1;//2 channels 1 min
    int totalword = totalbyte/2;
    int total = 0;
    int music_fd;
    int count;
    signed short applicbuf[2048];
    
    if ((music_fd = open(filename, O_WRONLY | O_CREAT, 0)) == -1) {
        perror(filename);
        exit(1);
    }
    
    /* 	recording three minutes
     * sample format :16bit
     * channel num: 2 channels
     * sample rate = speed
     * data rate = speed * 2 * 2	bytes/sec
     *
     */
    while (total != totalword) {        
        if(totalword - total >= 2048)
            count = 2048;
        else
            count = totalword - total;
        
        read(audio_fd, applicbuf, count);
        write(music_fd, applicbuf, count);
        total += count;
    }
}

int main(int argc, char** argv) {
    int fd;
    if(!analyze_parament(argc,argv)){
        exit(0);
    }
    if((fd=open(SOUND_DEV,omode,0)) == -1) {
	  perror(SOUND_DEV);
	  exit(1);
    }    
    set_pcm_parm(fd);
    
    if(tmode==PLAY){
        fprintf(stderr,"Start playing...\n");
        start_play(fd);
    }else{
        fprintf(stderr,"Start recoding...\n");
        start_recode(fd);
    }
    close(fd);
    return (EXIT_SUCCESS);
}

