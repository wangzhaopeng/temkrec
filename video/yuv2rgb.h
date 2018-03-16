
#ifndef __YUV2RGB_H__
#define __YUV2RGB_H__

#ifdef __cplusplus
extern "C"{
#endif

void yuv420_to_rgb24(int width, int height, unsigned char *pIn0, unsigned char *pOut0);

void yuyv_to_rgb24 (int width, int height, unsigned char *src, unsigned char *dst);

void uyvy_to_rgb24 (int width, int height, unsigned char *src, unsigned char *dst);

void yuv_to_yuv420 (int width, int height, unsigned char *src, unsigned char *dst);

bool YUV420toRGB24(unsigned char* yuvBuf, int nWidth, int nHeight, unsigned char * pRgbBuf);

bool RGB24toYUV420(unsigned char*  RgbBuf,int nWidth,int nHeight,unsigned char* yuvBuf);

#ifdef __cplusplus
}
#endif

#endif

