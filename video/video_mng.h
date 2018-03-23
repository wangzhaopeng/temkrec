#ifndef __VIDEO_MNG_H__
#define __VIDEO_MNG_H__

#include <time.h>
#include <pthread.h>

#include <vector>
#include <deque>

enum E_RECSTATE {
	E_RECSTATE_IDLE=0,
	E_RECSTATE_CMD,
	E_RECSTATE_INIT,
	E_RECSTATE_RUN,
	E_RECSTATE_AACOK,
	E_RECSTATE_MP4_ADD_AAC,
	E_RECSTATE_DEINIT,
	E_RECSTATE_END
};

struct s_video_para{
	void*mp_cam264;
	//bool m_rec_flag;
	E_RECSTATE m_vRECSTATE;
	time_t m_rec_begin_syssec;
	int m_hadrec264sec;
	pthread_t m_thread;
};

struct s_snd_para{
	void *mp_pcm;
	void*mp_pcmqueue;
	void *mp_aac;
	std::deque<vector<char>> *mp_aacque;
	int m_hadencsec;
	E_RECSTATE m_sndRECSTATE;
	time_t m_rec_begin_syssec;
	pthread_t m_thread_pcm;
	pthread_t m_thread_aac;
};

class video_mng{
public:
	//video_mng(void){}
	~video_mng(void);
	int init(void);
	int de_init(void);

	int init_snd(void);
	void de_init_snd(void);

	void rec(time_t sec);
	void recmp4(int idx);
	void readpcm(int);
	void aacenc(int);
	int getaac(int sec,int &input_samples,std::vector<char>&v_decoder_info,std::deque<vector<char>>&dqv_aac);

	bool m_thread_exitflag;
	struct s_video_para m_video_para[2];
	struct s_snd_para m_snd_para;

	pthread_mutex_t m_mutex;
};

void tst_vmng(void);

#endif
