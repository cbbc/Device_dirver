/*****************************************/
/*                                       */
/*       ***   *       ****    ***       */
/*      *   *  *      *       *   *      */
/*      *****  *       ****   *****      */
/*      *   *  *           *  *   *      */
/*      *   *  *****   ****   *   *      */
/*                                       */
/*****************************************/


/*****************************************************************************/
/*                               Include Files                               */
/*****************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <alsa/asoundlib.h>

#include "../Header/record_play_alsa.h"

/*****************************************************************************/
/*                             C++ Compatibility                             */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" { 
#endif

/*****************************************************************************/
/*                          Prototypes Definitions                           */
/*****************************************************************************/

int alsa_main(void);													// Main ALSA function
void alsa_signal_stop(int sig);											// Signal interrupt
snd_pcm_t *alsa_open(char *pcm_device, int capture_playback,
					 alsa_audio_parameters *audio_parameters);			// Open ALSA handle
int alsa_set_parameters(snd_pcm_t *pcm_handle, int config_soft_param,
						alsa_audio_parameters *audio_parameters);		// Set ALSA parameters
int alsa_read_data(snd_pcm_t *handle, unsigned char *audio_buffer,
				   int nb_samples);										// Read ALSA data
int alsa_write_data(snd_pcm_t *handle, unsigned char *audio_buffer,
					int nb_samples);									// Write ALSA data
void alsa_close(snd_pcm_t *handle);										// Close ALSA handle

/*****************************************************************************/
/*                           Variables definitions                           */
/*****************************************************************************/

char *alsa_sound_card = "default";
char alsa_running;

/*****************************************************************************/
/*                            Main ALSA function                             */
/*****************************************************************************/

int alsa_main(void)
{
	snd_pcm_t				*capture_handle;
	snd_pcm_t				*playback_handle;
	unsigned char			*audio_buffer;
	alsa_audio_parameters	audio_parameters;

	/*******************************/
	/* Initialize audio parameters */
	/*******************************/

	audio_parameters.format		= SND_PCM_FORMAT_U8;
	audio_parameters.rate		= 8000;
	audio_parameters.channels	= 1;

	/*******************************/
	/* Alloc memory for audio buffer */
	/*******************************/

	if ( audio_parameters.format == SND_PCM_FORMAT_U8 )
		audio_buffer = (unsigned char*)(malloc(ALSA_BUFFER_SIZE * audio_parameters.channels * (sizeof(char))));

	if ( audio_parameters.format == SND_PCM_FORMAT_S16_LE )
		audio_buffer = (unsigned char*)(malloc(ALSA_BUFFER_SIZE * audio_parameters.channels * 2 * (sizeof(char))));

	/***********************/
	/* Signal configuation */
	/***********************/

	alsa_running = 1;
	signal(SIGINT, alsa_signal_stop);

	/******************************************************************/
	/* Open capture and playback ALSA handle and set audio parameters */
	/******************************************************************/

	if ( (capture_handle = alsa_open(alsa_sound_card, SND_PCM_STREAM_CAPTURE, &audio_parameters)) == NULL )
		return EXIT_FAILURE;


	if ( (playback_handle = alsa_open(alsa_sound_card, SND_PCM_STREAM_PLAYBACK, &audio_parameters)) == NULL )
		return EXIT_FAILURE;

	/*****************************/
	/* Read and write audio data */
	/*****************************/

	while ( alsa_running )
	{
		if ( alsa_read_data(capture_handle, audio_buffer, ALSA_BUFFER_SIZE) < 0 )
			break;

		if ( alsa_write_data(playback_handle, audio_buffer, ALSA_BUFFER_SIZE) < 0 )
			break;
	}

	/*********************/
	/* Close ALSA handle */
	/*********************/

	alsa_close(capture_handle);
	alsa_close(playback_handle);

	/*********************/
	/* Free audio buffer */
	/*********************/

	free(audio_buffer);

	printf("!!!   BYE BYE   !!!\n");
	return EXIT_SUCCESS;
}

/*****************************************************************************/
/*                             Signal interrupt                              */
/*****************************************************************************/

void alsa_signal_stop(int sig)
{
	printf("\nAborted by signal %s...\n", strsignal(sig));
	alsa_running = 0;
}

/*****************************************************************************/
/*                             Open ALSA handle                              */
/*****************************************************************************/

snd_pcm_t *alsa_open(char *pcm_device, int capture_playback, alsa_audio_parameters *audio_parameters)
{
	snd_pcm_t	*pcm_handle;

	/*******************/
	/* Test parameters */
	/*******************/

	if ( (audio_parameters->format != SND_PCM_FORMAT_U8) && (audio_parameters->format != SND_PCM_FORMAT_S16_LE) )
	{
		printf("Bad format, please choice 'SND_PCM_FORMAT_U8' or 'SND_PCM_FORMAT_S16_LE'\n");
		return NULL;
	}

	if ( (audio_parameters->channels != 1) && (audio_parameters->channels != 2) )
	{
		printf("Bad number of channels, please choice '1' or '2'\n");
		return NULL;
	}

	/********************/
	/* Print parameters */
	/********************/

	if ( capture_playback == SND_PCM_STREAM_CAPTURE )
		printf("alsa_open_capture: opening %s at %iHz, ", pcm_device, audio_parameters->rate);
	else if ( capture_playback == SND_PCM_STREAM_PLAYBACK )
		printf("alsa_open_playback: opening %s at %iHz, ", pcm_device, audio_parameters->rate);

	if ( audio_parameters->format == SND_PCM_FORMAT_U8 )
		printf("format=SND_PCM_FORMAT_U8, stereo=%i\n", audio_parameters->channels);
	else if ( audio_parameters->format == SND_PCM_FORMAT_S16_LE )
		printf("format=SND_PCM_FORMAT_S16_LE, stereo=%i\n", audio_parameters->channels);

	/************/
	/* Open PCM */
	/************/

	if ( snd_pcm_open(&pcm_handle, pcm_device, capture_playback, 0) < 0 )
	{
		if ( capture_playback == SND_PCM_STREAM_CAPTURE )
			printf("alsa_open_capture: Error opening PCM device %s\n", pcm_device );
		else if ( capture_playback == SND_PCM_STREAM_PLAYBACK )
			printf("alsa_open_playback: Error opening PCM device %s\n", pcm_device );

		return NULL;
	}

	/******************/
	/* Set parameters */
	/******************/

	if ( capture_playback == SND_PCM_STREAM_CAPTURE )
	{
		if ( alsa_set_parameters(pcm_handle, 0, audio_parameters) < 0 )
		{
			snd_pcm_close(pcm_handle);
			return NULL;
		}
	}
	else if ( capture_playback == SND_PCM_STREAM_PLAYBACK )
	{
		if ( alsa_set_parameters(pcm_handle, 1, audio_parameters) < 0 )
		{
			snd_pcm_close(pcm_handle);
			return NULL;
		}
	}

	return pcm_handle;
}

/*****************************************************************************/
/*                            Set ALSA parameters                            */
/*****************************************************************************/

int alsa_set_parameters(snd_pcm_t *pcm_handle, int config_soft_param, alsa_audio_parameters *audio_parameters)
{
	int					dir;	
	int					periods;
	int					period_size;
	int					err;
	uint				exact_uvalue;
	unsigned long		exact_ulvalue;
	snd_pcm_uframes_t	boundary;
	snd_pcm_hw_params_t	*hw_params;
	snd_pcm_sw_params_t	*sw_params;

	/****************************/
	/* Variables initialization */
	/****************************/

	periods		= ALSA_PERIODS;
	period_size	= ALSA_PERIOD_SIZE;
	hw_params	= NULL;
	sw_params	= NULL;

	/***********************************************************/
	/* Allocate the snd_pcm_hw_params_t structure on the stack */
	/***********************************************************/

	if ( (err = snd_pcm_hw_params_malloc(&hw_params)) < 0 )
	{
		printf("alsa_set_params function: Can't allocate hardware parameter structure (%s)\n", snd_strerror(err));
		return -1;
	}

	/***********************************************/
	/* Init hwparams with full configuration space */
	/***********************************************/

	if ( snd_pcm_hw_params_any(pcm_handle, hw_params) < 0 )
	{
		printf("alsa_set_params function: Can't configure this PCM device\n");
		return -1;
	}
	
	if ( snd_pcm_hw_params_set_access(pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0 )
	{
		printf("alsa_set_params function: Error setting access\n");
		return -1;
	}

	/*********************/
	/* Set sample format */
	/*********************/

	if ( snd_pcm_hw_params_set_format(pcm_handle, hw_params, audio_parameters->format ) < 0 )
	{
		printf("alsa_set_params function: Error setting format\n");
		return -1;
	}

	/*******************************************************/
	/* Set sample rate. If the exact rate is not supported */
	/* by the hardware, use nearest possible rate.         */
	/*******************************************************/

	exact_uvalue = audio_parameters->rate;
	dir = 0;

	if ( (err = snd_pcm_hw_params_set_rate_near(pcm_handle, hw_params, &exact_uvalue, &dir)) < 0 )
	{
		printf("alsa_set_params function: Error setting rate to %i: %s\n", audio_parameters->rate, snd_strerror(err));
		return -1;
	}

	if ( dir != 0 )
	{
		printf( "alsa_set_params function: The rate %d Hz is not supported by your hardware\n"
				"==> Using %d Hz instead\n", audio_parameters->rate, exact_uvalue);
	}

	/**************************/
	/* Set number of channels */
	/**************************/

	if ( snd_pcm_hw_params_set_channels(pcm_handle, hw_params, audio_parameters->channels) < 0 )
	{
		printf("alsa_set_params function: Error setting channels\n");
		return -1;
	}

	/************************************************/
	/* Choose greater period size when rate is high */
	/************************************************/

	period_size = period_size * (audio_parameters->rate / 8000);	

	/******************************************************************/
	/* Set buffer size (in frames). The resulting latency is given by */
	/* latency = periodsize * periods / (rate * bytes_per_frame)      */
	/* set period size                                                */
	/******************************************************************/

	exact_ulvalue = period_size;
	dir = 0;

	if ( snd_pcm_hw_params_set_period_size_near(pcm_handle, hw_params, &exact_ulvalue, &dir) < 0 )
	{
		printf("alsa_set_params function: Error setting period size\n");
		return -1;
	}

	if ( dir != 0 )
	{
		printf( "alsa_set_params: The period size %d is not supported by your hardware\n"
				"==> Using %d instead\n", period_size, (int)exact_ulvalue);
	}

	period_size = exact_ulvalue;

	/**************************************************************/
	/* Set number of periods. Periods used to be called fragments */
	/**************************************************************/

	exact_uvalue = periods;
	dir = 0;

	if ( snd_pcm_hw_params_set_periods_near(pcm_handle, hw_params, &exact_uvalue, &dir) < 0 )
	{
		printf("alsa_set_params function: Error setting periods\n");
		return -1;
	}

	if ( dir != 0 )
	{
		printf( "alsa_set_params function: The number of periods %d is not supported by your hardware\n"
				"==> Using %d instead\n", periods, exact_uvalue);
	}

	/**********************************/
	/* Apply HW parameter settings to */
	/* PCM device and prepare device  */
	/**********************************/

	if ( (err = snd_pcm_hw_params(pcm_handle, hw_params)) < 0 )
	{
		printf("alsa_set_params function: Error setting HW params: %s\n", snd_strerror(err));
		return -1;
	}

	/*********************/
	/* Prepare sw params */
	/*********************/

	if ( config_soft_param )
	{
		snd_pcm_sw_params_alloca(&sw_params);
		snd_pcm_sw_params_current(pcm_handle, sw_params);

//		if ( (err = snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, period_size * 2)) < 0 )
		if ( (err = snd_pcm_sw_params_set_start_threshold(pcm_handle, sw_params, 1)) < 0 )
			printf("alsa_set_params function: Error setting start threshold: %s\n", snd_strerror(err));

		/* If stop threshold is equal to boundary then automatic stop will be disabled */
		if ( (err = snd_pcm_sw_params_get_boundary(sw_params, &boundary)) < 0 )
			printf("alsa_set_params function: Error getting boundary: %s\n", snd_strerror(err));
  
//		if ( (err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, sw_params, period_size * periods)) < 0 )
		if ( (err = snd_pcm_sw_params_set_stop_threshold(pcm_handle, sw_params, boundary)) < 0 )
			printf("alsa_set_params function: Error setting stop threshold: %s\n", snd_strerror(err));

		if ( ( err = snd_pcm_sw_params(pcm_handle, sw_params)) < 0 )
		{
			printf("alsa_set_params function: Error setting SW params: %s\n", snd_strerror(err));
			return -1;
		}
	}

	/****************************/
	/* free hardware parameters */
	/****************************/

	snd_pcm_hw_params_free(hw_params);

	/***********************/
	/* Prepare PCM for use */
	/***********************/

	if ( (err = snd_pcm_prepare(pcm_handle)) < 0 )
	{
		printf("alsa_set_params function: Can't prepare audio interface for use (%s)\n", snd_strerror (err));
		return -1;
	}

	return 0;	
}

/*****************************************************************************/
/*                              Read ALSA data                               */
/*****************************************************************************/

int alsa_read_data(snd_pcm_t *handle, unsigned char *audio_buffer, int nb_samples)
{
	int err;

	err = snd_pcm_readi(handle, audio_buffer, nb_samples);

	if ( err < 0 )
	{
		if ( err == -EBADFD )
			printf("alsa_read_data function: PCM is not in the right state: %s\n", snd_strerror(err));
		else if ( err == -EPIPE )
			printf("alsa_read_data function: An overrun occurred: %s\n", snd_strerror(err));
		else if ( err == -ESTRPIPE )
			printf("alsa_read_data function: A suspend event occurred: %s\n", snd_strerror(err));
	}
	else if ( err == 0 )
		printf("alsa_read_data function: Read 0 data\n");

	return err;
}

/*****************************************************************************/
/*                              Write ALSA data                              */
/*****************************************************************************/

int alsa_write_data(snd_pcm_t *handle, unsigned char *audio_buffer, int nb_samples)
{
	int err;

	err = snd_pcm_writei(handle, audio_buffer, nb_samples);

	if ( err < 0 )
	{
		if ( err == -EBADFD )
			printf("alsa_write_data function: PCM is not in the right state: %s\n", snd_strerror(err));
		else if ( err == -EPIPE )
			printf("alsa_write_data function: An overrun occurred: %s\n", snd_strerror(err));
		else if ( err == -ESTRPIPE )
			printf("alsa_write_data function: A suspend event occurred: %s\n", snd_strerror(err));
	}
	else if ( err == 0 )
		printf("alsa_write_data function: Write 0 data\n");

	return err;
}

/*****************************************************************************/
/*                             Close ALSA handle                             */
/*****************************************************************************/

void alsa_close(snd_pcm_t *handle)
{
	if ( handle )
		snd_pcm_close(handle);
}

/*****************************************************************************/
/*                             C++ Compatibility                             */
/*****************************************************************************/

#ifdef __cplusplus
}
#endif

/*****************************************************************************/
