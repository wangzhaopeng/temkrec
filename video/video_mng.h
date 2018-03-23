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

class video_mng{
public:
	//video_mng(void){}
	~video_mng(void);
	int init(void);
	int de_init(void);

	int rec(time_t sec);
	void recmp4(int idx);

	bool m_thread_exitflag;
	struct s_video_para m_video_para[2];
	void *mp_pcm_aac;

	pthread_mutex_t m_mutex;
};

void tst_vmng(void);

#endif
