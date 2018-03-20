#ifndef __H264_FRAME_MNG_H__
#define __H264_FRAME_MNG_H__

#include <pthread.h>
#include <vector>
#include <map>
#include <memory>

typedef std::vector<unsigned char> TYPE_VU8;
typedef std::shared_ptr<TYPE_VU8> TYPE_SPVU8;

class frame_mng{
public:
	frame_mng(int t_sec = 30);
	~frame_mng(void);
	int init(void);
	int de_init(void);

	void add_frame(int sec,TYPE_SPVU8 frame);
	int get_sec_frame(int sec, std::vector<TYPE_VU8>& v_sec);


	int m_que_sec;
	int m_cursec;
	std::vector<TYPE_SPVU8> mv_cursec;
	std::map<int, std::vector<TYPE_SPVU8>> m_fmap;

	pthread_mutex_t m_mutex;
};

#endif



