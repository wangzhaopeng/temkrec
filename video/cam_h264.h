#ifndef __CAM_H264_H__
#define __CAM_H264_H__

#include <string>
#include <thread>
#include <vector>
#include "pthread.h"


class cam_h264{
public:
	cam_h264(const char *dev, int width, int height, int frame_rate);
	~cam_h264(void);
	int init(void);
	void de_init(void);
	int run(void);
	int get_sps(std::vector<unsigned char>&sps);
	int get_pps(std::vector<unsigned char>&pps);
	int get_sec(int sec,std::vector<std::vector<unsigned char>> &v_sec);


	int m_w;
	int m_h;
	int m_f;
	std::string m_dev;
	void* mp_thread;
	pthread_t m_thread;
	bool m_thread_exitflag;

	void*mp_cam;
	void*mp_vpu;
	void*mp_h264queue;
};

void tst_cam264(void);

#endif
