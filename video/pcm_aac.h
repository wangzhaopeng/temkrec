
#ifndef __PCM_AAC_H__
#define __PCM_AAC_H__

#include "time.h"
#include <vector>
#include <deque>

class pcm_aac{
public:
	pcm_aac(int hz, int channal, int bits,int tsec);
	~pcm_aac(void);
	int init(void);
	void de_init(void);
	int enc_cmd(time_t sec);
	int getaac(int sec,int &input_samples,std::vector<char>&v_decoder_info,std::deque<vector<char>>&dqv_aac);

private:

	int m_channel;
	int m_hz;
	int m_bits;
	int m_total_sec;

	void *mp_pcm;
	void*mp_pcmqueue;
	std::deque<vector<char>> m_aacque;
	int m_aacinput_samples;
	std::vector<char>mv_decoder_info;
	int m_read_times;
	int m_hadencsec;
	time_t m_rec_begin_syssec;

	bool m_thread_exitflag;
	//int m_sndRECSTATE;
	int m_aacencstate;

	pthread_t m_thread_pcm;
	pthread_t m_thread_aac;
	void readpcm(int);
	void aacenc(int);
	pthread_mutex_t m_mutex;
};

void tst_pcmaac(void);

#endif
