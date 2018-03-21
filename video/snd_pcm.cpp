
#include <alsa/asoundlib.h>

#include "stdio.h"
#include "iostream"
using namespace std;


#include "snd_pcm.h"


snd_pcm::snd_pcm(int nSampleRate, int nChannal, int bitsPerSample){
	m_rate=nSampleRate;
	m_channel=nChannal;
	m_bit=bitsPerSample;
}

snd_pcm::~snd_pcm(){
	de_init();
}

void snd_pcm::de_init(){
	if(handle){
		snd_pcm_drain(handle);
		snd_pcm_close(handle);
	}
	if(buffer){
		free(buffer);
	}

	handle = NULL;
	buffer = NULL;
}


////return -1 err     0 ok
int snd_pcm::init(){
	handle = NULL;
	buffer = NULL;

	//long loops;
	int rc;
	//snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	unsigned int val;
	int dir;

	/* Open PCM device for playback. */
	//rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_PLAYBACK, 0);
	rc = snd_pcm_open(&handle, "default",SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0){
		fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
		handle = NULL;
		return -1;
	}
	/* Allocate a hardware parameters object. */
	snd_pcm_hw_params_alloca(&params);
	/* Fill it in with default values. */
	snd_pcm_hw_params_any(handle, params);
	/* Set the desired hardware parameters. */
	/* Interleaved mode */
	snd_pcm_hw_params_set_access(handle, params,SND_PCM_ACCESS_RW_INTERLEAVED);
	/* Signed 16-bit little-endian format */
	snd_pcm_hw_params_set_format(handle, params,SND_PCM_FORMAT_S16_LE);
	/* Two channels (stereo) */
	snd_pcm_hw_params_set_channels(handle, params, m_channel);
	/* 44100 bits/second sampling rate (CD quality) */
	val = m_rate;
	snd_pcm_hw_params_set_rate_near(handle, params,&val, &dir);
	/* Set period size to 32 frames. */
	frames = 32;
	snd_pcm_hw_params_set_period_size_near(handle,
		params, &frames, &dir);
	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		fprintf(stderr,"unable to set hw parameters: %s\n",snd_strerror(rc));
		return -1;//exit(1);
	}
	/* Use a buffer large enough to hold one period */
	snd_pcm_hw_params_get_period_size(params, &frames,&dir);
	size = frames * 2;//size = frames * 4; /* 2 bytes/sample, 2 channels */
	buffer = (char *)malloc(size);
	/* We want to loop for 5 seconds */
	snd_pcm_hw_params_get_period_time(params, &val, &dir);

	return 0;
}

////return -1 err     0 ok
int snd_pcm::read(){
	int iret = 0;
	int rc;
	rc = snd_pcm_readi(handle, buffer, frames);
	if (rc == -EPIPE) {
		/* EPIPE means overrun */
		fprintf(stderr, "overrun occurred\n");
		snd_pcm_prepare(handle);
		iret = -1;
	}else if (rc < 0) {
		fprintf(stderr,"error from read: %s\n",	snd_strerror(rc));
		iret = -1;
	}else if (rc != (int)frames) {
		fprintf(stderr, "short read, read %d frames\n", rc);
		iret = -1;
	}
	return iret;
}

#include <time.h>
void tst_pcm(void)
{
	snd_pcm m_pcm(22050,1);
	m_pcm.init();cout <<"init"<<endl;
	m_pcm.de_init();cout <<"de_init"<<endl;
	m_pcm.init();cout <<"init"<<endl;
	time_t bsec = time(NULL);
	int idx = 0;
	while(time(NULL)-bsec<2){
		m_pcm.read();cout<<time(NULL)<<" "<<idx++<<endl;
	}
	cout << m_pcm.size<<endl;
}


