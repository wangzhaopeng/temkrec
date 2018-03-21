#ifndef __VIDEO_MNG_H__
#define __VIDEO_MNG_H__

#include <time.h>
#include <pthread.h>

struct s_video_para{
	void*mp_cam264;
	bool m_rec_flag;
	time_t m_rec_sec;
	pthread_t m_thread;
};

struct s_snd_para{
	void *mp_pcm;
	void *mp_aac;
	void*mp_pcmqueue;
	void*mp_aacqueue;
	pthread_t m_thread;
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
	void aacenc(int);

	bool m_thread_exitflag;
	struct s_video_para m_video_para[2];
	struct s_snd_para m_snd_para;
};

void tst_vmng(void);

#endif
