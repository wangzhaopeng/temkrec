
#ifndef __YUVADD_H__
#define __YUVADD_H__

#ifdef __cplusplus
extern "C"{
#endif

struct yuv_frame
{
	int width;
	int height;
	unsigned char *pdata;
};

struct point_xy
{
	int w,h;
};

void yuv420_add(struct yuv_frame* pdst_frame, struct point_xy dst_point,struct yuv_frame*padd_frame);

#ifdef __cplusplus
}
#endif

#endif

