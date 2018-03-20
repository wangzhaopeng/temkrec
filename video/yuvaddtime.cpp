
#include <time.h>

#include <opencv2/opencv.hpp>
using namespace std;
using namespace cv;


#include "yuvadd.h"
#include "yuvaddtime.h"
#include "yuv2rgb.h"
#include "rgb_bmp.h"

#include <iostream>

static void imgaddtxt(IplImage *img,char *text)
{
	//char text[20] = "logo!";
	CvPoint point = cvPoint(15, 30);
	CvFont font;
	cvInitFont(&font,CV_FONT_HERSHEY_DUPLEX, 1,1);
	cvPutText(img, text, point, &font,CV_RGB(0, 0, 0) );
}


void yuv420addtime(unsigned char *pd,int w,int h,time_t sec,int ms)
{
	struct tm *local;
	local=localtime(&sec);
	char sztime[64] = {0};
	sprintf(sztime,"%04d-%02d-%02d %02d:%02d:%02d.%03d",
		local->tm_year+1900,
		local->tm_mon+1,
		local->tm_mday,
		local->tm_hour,
		local->tm_min,
		local->tm_sec,
		ms);

	IplImage *frame = NULL;  
	int txt_w = 480;
	int txt_h = 40;
	frame = cvCreateImage(cvSize(txt_w,txt_h),IPL_DEPTH_8U,3);
	memset(frame->imageData,255,txt_w*txt_h*3);
	imgaddtxt(frame,sztime);


	unsigned char *buf = new unsigned char[w*h*3];
	//static unsigned char yuv420buf_b[1024*1024*10];
	RGB24toYUV420((unsigned char *)(frame->imageData),frame->width,frame->height,buf);

	struct yuv_frame sback_frame;
	struct yuv_frame font_frame;
	sback_frame.width = w;
	sback_frame.height = h;
	sback_frame.pdata = pd;

	font_frame.width = frame->width;
	font_frame.height = frame->height;
	font_frame.pdata = buf;

	struct point_xy add_xy={10,10};
	yuv420_add(&sback_frame,add_xy,&font_frame);
	delete[] buf;

	//cvNamedWindow("frame");  
	//cvShowImage("frame", frame);  
	//cvWaitKey(1);

	cvReleaseImage(&frame);

	//{
	//	static unsigned char rgbbuf[1024*1024*10];
	//	YUV420toRGB24(pd,w,h,rgbbuf);
	//	save_rgb24_bmp((unsigned char *)rgbbuf,"stamp.bmp",w,h);
	//}
	//int a = 0;
	//a ++;
}
