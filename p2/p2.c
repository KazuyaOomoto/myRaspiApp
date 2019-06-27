#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <alsa/asoundlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include "p2msg.h"
#include "debug.h"
#include "os_wrapper.h"

#if 0
#define DEBUG (debug)
#else
#define DEBUG
#endif

/* PCM default settings */
#define DEF_CHANNEL         2
#define DEF_FS              48000
#define DEF_BITPERSAMPLE    16
#define WAVE_FORMAT_PCM     1
#define BUF_SAMPLES         1024
 
/* ChunkID definition */
const char ID_RIFF[4] = "RIFF";
const char ID_WAVE[4] = "WAVE";
const char ID_FMT[4]  = "fmt ";
const char ID_DATA[4] = "data";


static	snd_pcm_t *hndl	=	NULL;
static	int16_t *buffer	=	NULL;
static	FILE *fp		=	NULL;
static int      p2msqid;
static int      p2shmid;
static	P2msg	msgP2;

/* PCM information storage structure */
typedef struct {
    uint16_t      wFormatTag;         // format type  
    uint16_t      nChannels;          // number of channels (1:mono, 2:stereo)
    uint32_t      nSamplesPerSec;     // sample rate
    uint32_t      nAvgBytesPerSec;    // for buffer estimation
    uint16_t      nBlockAlign;        // block size of data
    uint16_t      wBitsPerSample;     // number of bits per sample of mono data
    uint16_t      cbSize;             // extra information
} WAVEFORMATEX;
 
/* CHUNK */
typedef struct {
    char        ID[4];  // Chunk ID
    uint32_t    Size;   // Chunk size;
} CHUNK;

/* SOUND FILE Manage	*/
typedef struct {
	const char*		pfname;  //sound file name
	void*			pformat;//file format
	int*			psize;	//file size
}SOUND_FILE_MNG;

static WAVEFORMATEX  wf0;
static WAVEFORMATEX  wf1;
static WAVEFORMATEX  wf2;
static WAVEFORMATEX  wf3;
static WAVEFORMATEX  wf4;
static WAVEFORMATEX  wf5;
static WAVEFORMATEX  wf6;
static int			dSize0;
static int			dSize1;
static int			dSize2;
static int			dSize3;
static int			dSize4;
static int			dSize5;
static int			dSize6;

static const SOUND_FILE_MNG const p2_sound_files[] = {
	{	"./resource/sound/syuttaikin_sentaku.wav",		(void*)&wf0,	&dSize0	},
	{	"./resource/sound/card_wo_kazasu.wav",			(void*)&wf1,	&dSize1	},
	{	"./resource/sound/card_nyuuryoku_sippai.wav",	(void*)&wf2,	&dSize2	},
	{	"./resource/sound/syukkin_simasita.wav",		(void*)&wf3,	&dSize3	},
	{	"./resource/sound/taikin_simasita.wav",			(void*)&wf4,	&dSize4	},
	{	"./resource/sound/syukkin_zumi.wav",			(void*)&wf5,	&dSize5	},
	{	"./resource/sound/syukkin_minyuuryoku.wav",		(void*)&wf6,	&dSize6	}
};

/* PCM default Infomation */
static	const WAVEFORMATEX wf_default = { 
	WAVE_FORMAT_PCM,   // PCM
	DEF_CHANNEL,
	DEF_FS,
	DEF_FS * DEF_CHANNEL * (DEF_BITPERSAMPLE/8),
	(DEF_BITPERSAMPLE/8) * DEF_CHANNEL,
	DEF_BITPERSAMPLE,
	0
};

/* Output device */
static	const char *device = "default";
/* ALSA buffer time[msec] */
static const unsigned int latency = 50000;

static void p2_send_stop_notify(P2msg*);
static void p2_stop_streaming(void);
static void p2_sig(int);
static int ParseWavHeader(FILE*, WAVEFORMATEX*);
static int p2_init(void);

/*	Process2 main function	*/
int p2_main(void)
{
	/* Soft SRC enable / disable setting */
	unsigned int soft_resample = 1;
	/* PCM Infomation */
	WAVEFORMATEX wf;
	/* 16 bit signed */
	snd_pcm_format_t format = SND_PCM_FORMAT_S16;

	int dSize, reSize, ret, n;
	long				sound_id;
	char*				pos;
	struct	sigaction	sig={0};

	sig.sa_handler=p2_sig;
	sigaction(SIGINT,&sig,NULL);

	if((p2msqid=msg_create("./resource/p2msg",'a'))==-1){
		return 1;
	}

	if((p2shmid=shm_create("./resource/p2shm",'a',1))==-1){
		return 1;
	}
	DEBUG("p2msqid=%lu\n", p2msqid);
	DEBUG("p2shmid=%lu\n", p2shmid);

	/* Process2 initialize	*/
	if(!p2_init()){
		printf("P2 initialize error!\n");
		return 1;
	}

	while(1)
	{
		/* Waiting for message reception   */
		if(msgrcv(p2msqid,&msgP2,sizeof(msgP2)-sizeof(msgP2.mtype), P2_SOUND_PLAY_REQ, 0)==-1){
			perror("p2:msgrcv");
			sound_id = -1;
		}
		else
			sound_id = msgP2.sound_id;

		/* Check if the ID in the definition    */
		if((sound_id < 0) || (sound_id >= sizeof(p2_sound_files)/sizeof(*p2_sound_files))	)
		{
			DEBUG("guideid=%d\n", sound_id);
			msgP2.sound_id = -1;
			goto End;
		}

		/* Open sound file */
		fp = fopen(p2_sound_files[sound_id].pfname, "rb");
		if (fp == NULL) {
			DEBUG("Open error:n");
			goto End;
		}

		wf = *((WAVEFORMATEX*)p2_sound_files[sound_id].pformat);
		dSize = *(p2_sound_files[sound_id].psize);

		/* Check PCM format and output information */
		DEBUG("format : PCM, nChannels = %d, SamplePerSec = %d, BitsPerSample = %d\n",
				wf.nChannels, wf.nSamplesPerSec, wf.wBitsPerSample);

		/* Buffer preparation */
		buffer = malloc(BUF_SAMPLES * wf.nBlockAlign);
		if(buffer == NULL) {
			DEBUG("malloc error?n");
			goto End;
		}

		/* Open PCM stream for playback */
		ret = snd_pcm_open(&hndl, device, SND_PCM_STREAM_PLAYBACK, 0);
		if(ret != 0) {
			DEBUG( "Unable to open PCM?n" );
			goto End;
		}

		/* Set various parameters such as format and buffer size */
		ret = snd_pcm_set_params( hndl, format, SND_PCM_ACCESS_RW_INTERLEAVED, wf.nChannels,
		                  wf.nSamplesPerSec, soft_resample, latency);
		if(ret != 0) {
			DEBUG( "Unable to set format?n" );
			goto End;
		}

		/* Attach shared memory(Read Only) */
		pos = (char*)shmat(p2shmid, 0, SHM_RDONLY);
		for (n = 0; n < dSize; n += BUF_SAMPLES * wf.nBlockAlign) {
			/* If the data in the shared memory is other than 0, it is canceled during processing */
			if(*pos){
				DEBUG("p2:sound cancel\n");
				break;
			}
			/* Read PCM */
			fread(buffer, wf.nBlockAlign, BUF_SAMPLES, fp);

			/* PCM writing */
			reSize = (n < BUF_SAMPLES) ? n : BUF_SAMPLES;
			ret = snd_pcm_writei(hndl, (const void*)buffer, reSize);
			/* When the stream is stopped due to buffer underrun etc., recovery is attempted */
			if (ret < 0) {
				if( snd_pcm_recover(hndl, ret, 0 ) < 0 ) {
					DEBUG( "Unable to recover Stream." );
					goto End;
				}
			}
		}
		/* Output the PCM that has accumulated since the data output is complete */
		snd_pcm_drain(hndl);
End:
		/* Stop streaming */
		p2_stop_streaming();

		/* Detach shared memory */
		shmdt((void*)pos);

		/* Send guidance stop notification */
		p2_send_stop_notify(&msgP2);

	}

	return 0;
}

static void p2_send_stop_notify(P2msg* pmsg)
{
	DEBUG("p2_send_stop_notify(guideid=%d)\n", pmsg->sound_id);
	pmsg->mtype=P2_SOUND_STOP_NOTIFY;
	msgsnd(p2msqid, pmsg, sizeof(*pmsg)-sizeof(pmsg->mtype),0);
}

static void p2_stop_streaming(void)
{
	/* Close the stream */
	if (hndl != NULL) {
		snd_pcm_close(hndl);
		/*	prevent double free	*/
		hndl = NULL;
	}

	/* Close the sound file */
	if (fp != NULL) {
		fclose(fp);
		/*	prevent double free	*/
		fp = NULL;
	}

	/* Free memory */
	if (buffer != NULL) {
		free(buffer);
		/*	prevent double free	*/
		buffer = NULL;
	}
}

static void p2_sig(int signum)
{
	DEBUG("p2_sig\n");
	if(hndl)
		/* Output the PCM that has accumulated since the data output is complete */
		snd_pcm_drain(hndl);

	p2_stop_streaming();
	_exit(1);
}

/*   Analyze the header part of the WAVE file and put necessary information in the structure
 *  ÅEfp is pointed to the beginning of DATA
 *  ÅEReturn value: Success: Size of data Chunk, Failure: -1
 */
static int ParseWavHeader(FILE *fp, WAVEFORMATEX *wf)
{
	char  FormatTag[4];
	CHUNK Chunk;
	int ret = -1;
	int reSize;

    /* Read RIFF Chunk*/
	if((fread(&Chunk, sizeof(Chunk), 1, fp) != 1) ||
		(strncmp(Chunk.ID, ID_RIFF, 4) != 0)) {
		DEBUG("file is not RIFF Format ?n");
		goto RET;
	}

	/* Read Wave */
	if((fread(FormatTag, 1, 4, fp) != 4) ||
		(strncmp(FormatTag, ID_WAVE, 4) != 0)){
		DEBUG("file is not Wave file?n");
		goto RET;
	}

	/* Read Sub Chunk (Expect FMT, DATA) */
	while(fread(&Chunk, sizeof(Chunk), 1, fp) == 1) {
		if(strncmp(Chunk.ID, ID_FMT, 4) == 0) {
			/* Fit to the smaller (to care for cbSize) */
			reSize = (sizeof(WAVEFORMATEX) < Chunk.Size) ? sizeof(WAVEFORMATEX) : Chunk.Size;
			fread(wf, reSize, 1, fp);
			if(wf->wFormatTag != WAVE_FORMAT_PCM) {
			    DEBUG("Input file is not PCM?n");
			    goto RET;
			}
		}
		else if(strncmp(Chunk.ID, ID_DATA, 4) == 0) {
		/* If find a DATA Chunk, return its size */
			ret = Chunk.Size;
			break;
		}
		else {
			/* If find a unknown Chunk, skips reading */
			fseek(fp, Chunk.Size, SEEK_CUR);
			continue;
		}
	};

RET:
	return ret;
}

static int p2_init(void)
{
	int ret;
	int i;
	int dSize;
	FILE* fpl;
	WAVEFORMATEX* pwf;

	ret = 1;
	for(i = 0; i < sizeof(p2_sound_files)/sizeof(*p2_sound_files) && ret; i++ )
	{
		/* No sound file	*/
		if(!p2_sound_files[i].pfname) {ret = 0; continue;}
		DEBUG("p2_sound_files[%d] is a file\n", i);
		/* Open sound file */
		fpl = fopen(p2_sound_files[i].pfname, "rb");
		if (fpl == NULL) {ret = 0; continue;}	//the file is not exist.
		DEBUG("p2_sound_files[%d] is exist \n", i);
		
		/* default wav format setting	*/
		pwf = (WAVEFORMATEX*)p2_sound_files[i].pformat;
		*pwf = wf_default;

		/* Parse the WAV header */
		dSize = ParseWavHeader(fpl, pwf);
		if (dSize <= 0) {ret = 0; continue;}	//the file is not a wav format.
		DEBUG("p2_sound_files[%d] is correct wav format \n", i);

		*(p2_sound_files[i].psize) = dSize;
		DEBUG("p2_sound_files[%d] is valid\n", i);
	}

	return ret;
}
