
#ifndef __CAM_CAP_H__
#define __CAM_CAP_H__

#ifdef __cplusplus
extern "C"{
#endif





#ifdef __cplusplus
}
#endif




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
	cam_cap(char *dev,int width,int height,int frame_rate,int pix_fmt=V4L2_PIX_FMT_YUV420);//=V4L2_PIX_FMT_YUYV,V4L2_PIX_FMT_YUV420
	~cam_cap();
	int init();
	int query_frame(unsigned char *data);
private:
	int v4l_capture_setup(void);
	int start_capturing(void);


public:
	int m_width;
	int m_height;
	int m_frame_size;
	int m_frame_rate;
private:
	char m_dev_name[100];//v4l_capture_dev[100];
	int m_dev_h;
	int m_input;
	int m_fmt;
	int m_capture_num_buffers;
	struct data_buffer m_capture_buffers[4];
};





#endif

