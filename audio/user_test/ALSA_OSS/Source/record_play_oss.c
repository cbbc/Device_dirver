/***********************************/
/*                                 */
/*       ***    ****    ****       */
/*      *   *  *       *           */
/*      *   *   ****    ****       */
/*      *   *       *       *      */
/*       ***    ****    ****       */
/*                                 */
/***********************************/


	/*********************************************************/
	/*                                                       */
	/*                        DEVICES                        */
	/*                                                       */
	/*   "/dev/dsp"   -> used 8-bits                         */
	/*   "/dev/dspW"  -> used 16-bits signed little-endian   */
	/*   "/dev/audio" -> used mu-law                         */
	/*                                                       */
	/*********************************************************/
	/*                                                       */
	/*                       OPEN MODE                       */
	/*                                                       */
	/*   "O_WRONLY"  -> Write only for play                  */
	/*   "O_RD_ONLY" -> Read only for record                 */
	/*   "O_RDWR"    -> Read/Write for play and record       */
	/*                                                       */
	/*********************************************************/
	/*                                                       */
	/*                    SOURCE CAPTURE                     */
	/*                                                       */
	/*   "SOUND_MASK_PHONEIN" -> Headset                     */
	/*   "SOUND_MASK_MIC"     -> Handset                     */
	/*   "SOUND_MASK_LINE"    -> Line in                     */
	/*                                                       */
	/*********************************************************/


/*****************************************************************************/
/*                               Include Files                               */
/*****************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include "../Header/record_play_oss.h"

/*****************************************************************************/
/*                             C++ Compatibility                             */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" { 
#endif

/*****************************************************************************/
/*                          Prototypes Definitions                           */
/*****************************************************************************/

int oss_main(void);				// Main OSS function
void oss_signal_stop(int sig);	// Signal interrupt

/*****************************************************************************/
/*                           Variables definitions                           */
/*****************************************************************************/

char oss_running;

/*****************************************************************************/
/*                             Main OSS function                             */
/*****************************************************************************/

int oss_main(void)
{
	int				len;
	int				source;
	int				fd_audio;
	unsigned char	audio_buffer[OSS_BUF_SIZE];

	printf("Open Sound System\n");

	/* Signal configuation */
	oss_running = 1;
	signal(SIGINT, oss_signal_stop);

	/* Open device */
	if ( (fd_audio = open("/dev/dsp", O_RDWR, 0)) == -1 )
	{
		/* Open of device failed */
		printf("Error, can't open device /dev/dsp\n");
		return EXIT_FAILURE;
	}

	/* Select source capture */
	source = SOUND_MASK_MIC;
	if ( ioctl(fd_audio, SOUND_MIXER_READ_RECSRC, &source) < 0 )
	{
		printf("Error, can't select source capture\n");
		return EXIT_FAILURE;
	}

	/* Read and print source capture */
	ioctl(fd_audio, SOUND_MIXER_READ_RECSRC, &source);
	printf("Source record: %d\n", source);

	/* Loop */
	while ( oss_running )
	{
		/* Record */
		if ( (len = read(fd_audio, audio_buffer, 128)) == -1 )
		{
			printf("Error audio read\n");
			return EXIT_FAILURE;
		}

		/* Play */
		if ( (len = write(fd_audio, audio_buffer, 128)) == -1 )
		{
			printf("Error audio write\n");
			return EXIT_FAILURE;
		}
	}

	printf("!!!   BYE BYE   !!!\n");
	return EXIT_SUCCESS;
}

/*****************************************************************************/
/*                             Signal interrupt                              */
/*****************************************************************************/

void oss_signal_stop(int sig)
{
	printf("\nAborted by signal %s...\n", strsignal(sig));
	oss_running = 0;
}

/*****************************************************************************/
/*                             C++ Compatibility                             */
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/*****************************************************************************/
