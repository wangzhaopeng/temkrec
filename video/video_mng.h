#ifndef __VIDEO_MNG_H__
#define __VIDEO_MNG_H__

#include <time.h>
#include <pthread.h>
struct s_thread_recmp4{
	bool m_rec_flag;
	time_t m_rec_sec;
};

class video_mng{
public:
	//video_mng(void){}
	~video_mng(void);
	int init(void);
	int de_init(void);

	void rec(time_t sec);
	void recmp4(int idx);

	bool m_thread_exitflag;
	//bool m_rec_flag;
	//time_t m_rec_sec;


	struct s_thread_recmp4 m_recmp4_para[2];
	pthread_t m_thread_recmp4[2];

	void*mp_cam264_0;
	void*mp_cam264_1;
};

void tst_vmng(void);

#endif
