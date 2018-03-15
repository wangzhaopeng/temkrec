


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

#include "cam_cap.h"



cam_cap::cam_cap(char *dev,int width,int height,int frame_rate,int pix_fmt)
{
	snprintf(m_dev_name,sizeof(m_dev_name),"%s",dev);
	m_dev_h = 0;
	m_input = 1;
	m_fmt = pix_fmt;
	m_width = width;
	m_height = height;
	m_frame_rate = frame_rate;
	m_capture_num_buffers = 4;
}


cam_cap::~cam_cap()
{
	int i;
	enum v4l2_buf_type type;

	if(m_dev_h > 0){
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ioctl(m_dev_h, VIDIOC_STREAMOFF, &type);

		for (i = 0; i < m_capture_num_buffers; i++)
		{
			munmap(m_capture_buffers[i].start, m_capture_buffers[i].length);
		}
		close(m_dev_h);
	}
}


int cam_cap::v4l_capture_setup(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers req;
	//struct v4l2_dbg_chip_ident chip;
	struct v4l2_streamparm parm;
	v4l2_std_id id;
	int i;

#if 0
	if (ioctl (m_dev_h, VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n",
					m_dev_name);
			return -1;
		} else {
			fprintf (stderr, "%s isn not V4L device,unknow error\n",
			m_dev_name);
			return -1;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf (stderr, "%s is no video capture device\n",
			m_dev_name);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf (stderr, "%s does not support streaming i/o\n",
			m_dev_name);
		return -1;
	}

	if (ioctl(m_dev_h, VIDIOC_DBG_G_CHIP_IDENT, &chip))
	{
		printf("VIDIOC_DBG_G_CHIP_IDENT failed.\n");
		return -1;
	}
	printf("TV decoder chip is %s\n", chip.match.name);
#endif
//sleep(1);
//exit(0);

#if 1
	if (ioctl(m_dev_h, VIDIOC_S_INPUT, &m_input) < 0)
	{
		printf("VIDIOC_S_INPUT failed\n");
		return -1;
	}
#endif

#if 0
	for(i = 0; i < 3; i++)
	{

		if (ioctl(m_dev_h, VIDIOC_G_STD, &id) < 0)
		{
			printf("VIDIOC_G_STD failed\n");
			return -1;
		}
#if 0
		printf("%d\n",id);
		if(id==V4L2_STD_NTSC)
		{
			printf("std ntsc \n");
		}
		if(id==V4L2_STD_PAL)
		{
			printf("std pal \n");
		}
#endif
		if (id == V4L2_STD_ALL)
		{
			sleep(1);
			continue;
		}

	}
#endif

#if 0
	if (id == V4L2_STD_ALL)
	{
		printf("Cannot detect TV standard\n");
		return -1;
	}
id=V4L2_STD_NTSC;
	if (ioctl(m_dev_h, VIDIOC_S_STD, &id) < 0)
	{
		printf("VIDIOC_S_STD failed\n");
		return -1;
	}
#endif


#if 0
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	struct v4l2_fmtdesc fmt;
	struct v4l2_frmsizeenum frmsize;
	struct v4l2_frmivalenum frmival;
	fmt.index = 0;
	fmt.type = type;
	while(1){
		if(ioctl(m_dev_h,VIDIOC_ENUM_FMT,&fmt)>=0){
			printf("{ pixelformat = %c%c%c%c,description = %s }\n",fmt.pixelformat&0xff,(fmt.pixelformat>>8)&0xff,(fmt.pixelformat>>16)&0xff,(fmt.pixelformat>>24)&0xff,fmt.description);
			frmsize.pixel_format = fmt.pixelformat;
			frmsize.index = 0;
			while(1){
				if(ioctl(m_dev_h,VIDIOC_ENUM_FRAMESIZES,&frmsize)>=0){
					if(frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE){
						printf("%dx%d\n",frmsize.discrete.width,
							frmsize.discrete.height);
					}else if(frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE){
						printf("%dx%d\n",frmsize.stepwise.max_width,
							frmsize.stepwise.max_height);
					}
					frmsize.index++;
				}else{
					printf("-1 %d\n",__LINE__);
					break;
				}
			}
			//printf("%d.%s\n",fmt.index+1,fmt.description);
			fmt.index++;
		}else{
			printf("-1 %d\n",__LINE__);
			break;
		}
	}
}
#endif
	/* Select video input, video standard and tune here. */

#if 0
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl (m_dev_h, VIDIOC_CROPCAP, &cropcap) < 0) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (ioctl (m_dev_h, VIDIOC_S_CROP, &crop) < 0) {
			switch (errno) {
				case EINVAL:
					/* Cropping not supported. */
					fprintf (stderr, "%s  doesn't support crop\n",
						m_dev_name);
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	} else {
		/* Errors ignored. */
	}
#endif

#if 1
	if(m_width==320 && m_height==240){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 1;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0)
		{
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}else if(m_width==640 && m_height==480){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 0;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0)
		{
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}else if(m_width==1280 && m_height==720){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 4;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0)
		{
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}else if(m_width==1920 && m_height==1080){
		//printf("720p\n");
		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = m_frame_rate;//15;
		parm.parm.capture.capturemode = 5;
		if (ioctl(m_dev_h, VIDIOC_S_PARM, &parm) < 0)
		{
			printf("VIDIOC_S_PARM failed\n");
			return -1;
		}
	}


#endif

	memset(&fmt, 0, sizeof(fmt));

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = m_width;
	fmt.fmt.pix.height      = m_height;
	fmt.fmt.pix.pixelformat = m_fmt;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	if (ioctl (m_dev_h, VIDIOC_S_FMT, &fmt) < 0){
		fprintf (stderr, "%s iformat not supported \n",
			m_dev_name);
		return -1;
	}

	if (ioctl(m_dev_h, VIDIOC_G_FMT, &fmt) < 0)
	{
		printf("VIDIOC_G_FMT failed\n");
		return -1;
	}

	m_width = fmt.fmt.pix.width;
	m_height = fmt.fmt.pix.height;
	m_frame_size = fmt.fmt.pix.sizeimage;
	
	printf("capture width=%d height=%d size=%d\n", m_width, m_height, m_frame_size);

	memset(&req, 0, sizeof (req));

	req.count               = m_capture_num_buffers;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (ioctl (m_dev_h, VIDIOC_REQBUFS, &req) < 0) {
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
					 "memory mapping\n", m_dev_name);
			return -1;
		} else {
			fprintf (stderr, "%s does not support "
					 "memory mapping, unknow error\n", m_dev_name);
			return -1;
		}
	}

	if (req.count < 2) {
		fprintf (stderr, "Insufficient buffer memory on %s\n",
			 m_dev_name);
		return -1;
	}

	return 0;
}



int cam_cap::start_capturing(void)
{
        unsigned int i, j;
        struct v4l2_buffer buf;
        enum v4l2_buf_type type;

        for (i = 0; i < m_capture_num_buffers; i++)
        {
                memset(&buf, 0, sizeof (buf));
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                if (ioctl(m_dev_h, VIDIOC_QUERYBUF, &buf) < 0)
                {
                        printf("VIDIOC_QUERYBUF error\n");
			for (j = 0; j < i; j++)
			{
				munmap(m_capture_buffers[j].start, m_capture_buffers[j].length);
			}
                        return -1;
                }

                m_capture_buffers[i].length = buf.length;
                m_capture_buffers[i].offset = (size_t) buf.m.offset;
                m_capture_buffers[i].start = (unsigned char*)mmap (NULL, m_capture_buffers[i].length,
                    PROT_READ | PROT_WRITE, MAP_SHARED,m_dev_h, (off_t)m_capture_buffers[i].offset);
		memset(m_capture_buffers[i].start, 0xFF, m_capture_buffers[i].length);
	}

	for (i = 0; i < m_capture_num_buffers; i++)
	{
		memset(&buf, 0, sizeof (buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		buf.m.offset = m_capture_buffers[i].offset;
		if (ioctl (m_dev_h, VIDIOC_QBUF, &buf) < 0) {
			printf("VIDIOC_QBUF error\n");
			for (j = 0; j < m_capture_num_buffers; j++)
			{
				munmap(m_capture_buffers[j].start, m_capture_buffers[j].length);
			}
			return -1;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl (m_dev_h, VIDIOC_STREAMON, &type) < 0) {
		printf("VIDIOC_STREAMON error\n");
		for (j = 0; j < m_capture_num_buffers; j++)
		{
			munmap(m_capture_buffers[j].start, m_capture_buffers[j].length);
		}
		return -1;
	}

	return 0;
}



int cam_cap::init()
{
	v4l2_std_id id;
	int iret;

	m_dev_h = open(m_dev_name, O_RDWR, 0);
	if (m_dev_h < 0)
	{
		printf("Unable to open %s\n", m_dev_name);
		return m_dev_h;
	}

	iret = v4l_capture_setup();
	if (iret < 0) {
		printf("Setup v4l capture failed.\n");
		close(m_dev_h);
		return iret;
	}

	iret = start_capturing();
	if(iret < 0) {
		printf("start_capturing failed\n");
		close(m_dev_h);
		return iret;
	}

	return m_dev_h;
}

int cam_cap::query_frame(unsigned char *data)
{
	struct v4l2_buffer capture_buf;

	memset(&capture_buf, 0, sizeof(capture_buf));
	capture_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	capture_buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(m_dev_h, VIDIOC_DQBUF, &capture_buf) < 0) {
		printf("VIDIOC_DQBUF failed.\n");
		return -1;
	}
	
	struct timeval tv;
	gettimeofday(&tv,NULL);

	memcpy(data, m_capture_buffers[capture_buf.index].start, m_frame_size);

	struct timeval tv2;
	gettimeofday(&tv2,NULL);
	//printf("%ds %dms\n",tv2.tv_sec-tv.tv_sec,(tv2.tv_usec-tv.tv_usec)/1000);

	if (ioctl(m_dev_h, VIDIOC_QBUF, &capture_buf) < 0) {
		printf("VIDIOC_QBUF failed\n");
		return -1;
	}

	return 0;
}



