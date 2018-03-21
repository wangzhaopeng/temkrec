#ifndef _SND_PCM_H_
#define _SND_PCM_H_


#include <alsa/asoundlib.h>

class snd_pcm{
public:	
	~snd_pcm();
	snd_pcm(int nSampleRate, int nChannal, int bitsPerSample=16);
	int init();
	void de_init();
	int read();

//private:
	snd_pcm_t *handle;
	snd_pcm_uframes_t frames;
	char *buffer;
	int size;

	int m_rate,m_channel,m_bit;
};

void tst_pcm(void);

#endif
