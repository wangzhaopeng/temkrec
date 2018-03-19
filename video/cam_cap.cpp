
#include "stdio.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <sys/mman.h>

#include <string.h>
#include <malloc.h>
#include <sys/time.h>


#include <time.h>

#include <iostream>
#include <vector>
using namespace std;

#include "cam_cap.h"



cam_cap::cam_cap(const char *dev, int width, int height, int frame_rate, int pix_fmt){
	mstr_dev = dev;
	m_dev_h = -1;
	m_input = 1;
	m_fmt = pix_fmt;
	m_width = width;
	m_height = height;
	m_frame_rate = frame_rate;
	m_capture_num_buffers = 4;
	memset(m_capture_buffers,0,sizeof(m_capture_buffers));
	mb_onflag = false;
}

cam_cap::~cam_cap(){
	de_init();
}

void cam_cap::de_init(void){
	if(mb_onflag){
		enum v4l2_buf_type type;
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ioctl(m_dev_h, VIDIOC_STREAMOFF, &type);
		mb_onflag = false;
		cout << "VIDIOC_STREAMOFF "<<endl;
	}
	for (int i = 0; i < m_capture_num_buffers; i++){
		if(m_capture_buffers[i].start){
			munmap(m_capture_buffers[i].start, m_capture_buffers[i].length);
			memset(&m_capture_buffers[i],0,sizeof(m_capture_buffers[i]));
			//cout << "munmap: "<<i<<endl;
		}
	}
	if(m_dev_h>=0){
		cout << "close "<<mstr_dev<<" fd:"<<m_dev_h<<endl;
		close(m_dev_h);
		m_dev_h = -1;

	}
}

/////return 0 ok   -1 err
int cam_cap::init(){
	v4l2_std_id id;
	int iret;

	m_dev_h = open(mstr_dev.c_str(), O_RDWR, 0);
	if (m_dev_h < 0){
		printf("Unable to open %s\n",mstr_dev.c_str());
		de_init();
		return -1;
	}

	iret = v4l_capture_setup();
	if (iret < 0) {
		printf("Setup v4l capture failed.\n");
		de_init();
		return -1;
	}

	iret = start_capturing();
	if (iret < 0) {
		printf("start_capturing failed\n");
		de_init();
		return -1;
	}

	return 0;
}

/////return 0 ok   -1 err
int cam_cap::v4l_capture_setup(void){
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	struct v4l2_streamparm parm;
	v4l2_std_id id;
	int i;

#if 1
	if (ioctl(m_dev_h, VIDIOC_S_INPUT, &m_input) < 0){
		printf("VIDIOC_S_INPUT failed\n");
		return -1;
	}
#endif


#if 1
	if (m_width == 320 && m_height == 240){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 1;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0){
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}else if (m_width == 640 && m_height == 480){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 0;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0){
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}else if (m_width == 1280 && m_height == 720){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 4;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0){
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}else if (m_width == 1920 && m_height == 1080){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 5;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0){
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}

#endif

	memset(&fmt, 0, sizeof(fmt));

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = m_width;
	fmt.fmt.pix.height = m_height;
	fmt.fmt.pix.pixelformat = m_fmt;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (ioctl(m_dev_h, VIDIOC_S_FMT, &fmt) < 0){
		fprintf(stderr, "%s iformat not supported \n",mstr_dev.c_str());
		return -1;
	}

	if (ioctl(m_dev_h, VIDIOC_G_FMT, &fmt) < 0){
		printf("VIDIOC_G_FMT failed\n");
		return -1;
	}

	m_width = fmt.fmt.pix.width;
	m_height = fmt.fmt.pix.height;
	m_frame_size = fmt.fmt.pix.sizeimage;

	printf("capture width=%d height=%d size=%d\n", m_width, m_height, m_frame_size);

	memset(&req, 0, sizeof(req));

	req.count = m_capture_num_buffers;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (ioctl(m_dev_h, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support memory mapping\n", mstr_dev.c_str());
			return -1;
		}else {
			fprintf(stderr, "%s does not support memory mapping, unknow error\n", mstr_dev.c_str());
			return -1;
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",mstr_dev.c_str());
		return -1;
	}

	return 0;
}

/////return 0 ok   -1 err
int cam_cap::start_capturing(void){
	unsigned int i, j;
	struct v4l2_buffer buf;
	enum v4l2_buf_type type;

	for (i = 0; i < m_capture_num_buffers; i++){
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		if (ioctl(m_dev_h, VIDIOC_QUERYBUF, &buf) < 0){
			printf("VIDIOC_QUERYBUF error\n");
			return -1;
		}

		m_capture_buffers[i].length = buf.length;
		m_capture_buffers[i].offset = (size_t)buf.m.offset;
		m_capture_buffers[i].start = (unsigned char*)mmap(NULL, m_capture_buffers[i].length,
			PROT_READ | PROT_WRITE, MAP_SHARED, m_dev_h, (off_t)m_capture_buffers[i].offset);
		//memset(m_capture_buffers[i].start, 0xFF, m_capture_buffers[i].length);
	}

	for (i = 0; i < m_capture_num_buffers; i++){
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.m.offset = m_capture_buffers[i].offset;
		if (ioctl(m_dev_h, VIDIOC_QBUF, &buf) < 0) {
			printf("VIDIOC_QBUF error\n");
			return -1;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(m_dev_h, VIDIOC_STREAMON, &type) < 0) {
		printf("VIDIOC_STREAMON error\n");
		return -1;
	}

	mb_onflag = true;

	cout << "VIDIOC_STREAMON ok"<<endl;
	return 0;
}

/////return 0 ok   -1 err
int cam_cap::query_frame(void *data){
	void*p;
	int iret = query_frame_p(&p);
	if(iret<0){
		return -1;
	}
	if(data!=NULL){
		memcpy(data, p, m_frame_size);
	}
	return 0;
}

/////return 0 ok   -1 err
int cam_cap::query_frame_p(void **pp){
	struct v4l2_buffer capture_buf;

	memset(&capture_buf, 0, sizeof(capture_buf));
	capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capture_buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(m_dev_h, VIDIOC_DQBUF, &capture_buf) < 0) {
		printf("VIDIOC_DQBUF failed.\n");
		return -1;
	}

	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	if(pp!=NULL){
		*pp = m_capture_buffers[capture_buf.index].start;
	}

	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	//cout << tv2.tv_sec-tv.tv_sec<<" sec "<< (tv2.tv_usec-tv.tv_usec)/1000<<" ms"<<endl;

	if (ioctl(m_dev_h, VIDIOC_QBUF, &capture_buf) < 0) {
		printf("VIDIOC_QBUF failed\n");
		return -1;
	}
	return 0;
}

#include "yuv2rgb.h"
#include "rgb_bmp.h"
void test_cam(void)
{
	int w = 640,h=480,f=30;
	char *dev = "/dev/video0";

	cam_cap cam(dev,w,h,f);cout << w<<h<<endl;
	cam.init();

	cam.query_frame(NULL);
	cam.de_init();
	cam.init();

	int idx = 0;

	vector<unsigned char> vuc(1920*1080*4);
	vector<unsigned char> vucrgb(1920*1080*4);
	time_t last_rec = 0;

	while(1){
		int iret;
		time_t sec = time(NULL);
		cout <<"frame "<< idx++ <<" "<< ctime(&sec);

		if(time(NULL) - last_rec >= 60){
			last_rec = time(NULL);
			iret = cam.query_frame(&vuc[0]);

			YUV420toRGB24(&vuc[0], w, h, &vucrgb[0]);
			char fn[64];
			sprintf(fn,"%d.bmp",idx);
			save_rgb24_bmp(&vucrgb[0], fn, w, h);
		}else{
			iret = cam.query_frame(NULL);
		}
		
		if(iret < 0){
			break;
		}
	}
}


