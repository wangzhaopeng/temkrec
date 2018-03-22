
#ifndef __CAM_CAP_H__
#define __CAM_CAP_H__

#include <time.h>
#include <string>
#include <vector>


struct data_buffer
{
	unsigned char *start;
	size_t offset;
	unsigned int length;
};

#include <linux/videodev2.h>


class cam_cap 
{
public:
	cam_cap(const char *dev,int width,int height,int frame_rate,int pix_fmt=V4L2_PIX_FMT_YUV420);//=V4L2_PIX_FMT_YUYV,V4L2_PIX_FMT_YUV420
	~cam_cap();
	int init();
	void de_init();
	int query_frame(void *data);
	int query_frame(void*pvpu,time_t &sec,std::vector<unsigned char> &v_h264);
private:
	int v4l_capture_setup(void);
	int start_capturing(void);
	int enc264(void*pvpu,unsigned char*pd,time_t &sec,std::vector<unsigned char> &v_h264);

public:
	int m_width;
	int m_height;
	int m_frame_size;
	int m_frame_rate;
private:
	std::string mstr_dev;
	int m_dev_h;
	int m_input;
	int m_fmt;
	int m_capture_num_buffers;
	struct data_buffer m_capture_buffers[4];
	bool mb_onflag;
	time_t m_last_sec;
};

void test_cam(void);

#endif

