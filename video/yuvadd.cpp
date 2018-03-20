
#include "stdio.h"
#include "string.h"

#include "yuvadd.h"


static void cpy_cur_yline(struct yuv_frame* pdst_frame, struct point_xy dst_point,struct yuv_frame*padd_frame,int line)
{
	unsigned char*pdsty,*p_curdsty;
	int dst_w;
	dst_w = pdst_frame->width;
	pdsty = pdst_frame->pdata;
	p_curdsty = pdsty + (dst_point.h+line)*dst_w + dst_point.w;


	unsigned char*paddy,*p_curaddy;
	int add_w;
	add_w = padd_frame->width;
	paddy = padd_frame->pdata;
	p_curaddy = paddy+line*add_w;

	int cp_size;
	cp_size = add_w;
	memcpy(p_curdsty,p_curaddy,cp_size);
}

static void cpy_cur_uline(struct yuv_frame* pdst_frame, struct point_xy dst_point,struct yuv_frame*padd_frame,int line)
{
	if (line%2!=0)
	{
		return;
	}

	unsigned char*pdsty, *pdstu, *pdstv,*p_curdstu;
	int dst_w,dst_h;
	dst_w = pdst_frame->width;
	dst_h = pdst_frame->height;
	pdsty = pdst_frame->pdata;
	pdstv = pdst_frame->pdata + dst_w * dst_h;
	pdstu = pdstv + dst_w/2 * dst_h/2;
	p_curdstu = pdstu + (dst_point.h+line)/2*dst_w/2 + dst_point.w/2;

	unsigned char*paddy, *paddu, *paddv,*p_curaddu;
	int add_w,add_h;
	add_w = padd_frame->width;
	add_h = padd_frame->height;
	paddy = padd_frame->pdata;
	paddv = padd_frame->pdata + add_w * add_h;
	paddu = paddv + add_w/2 * add_h/2;
	p_curaddu = paddu + line/2*add_w/2;

	int cp_size;
	cp_size = add_w/2;
	memcpy(p_curdstu,p_curaddu,cp_size);
}

static void cpy_cur_vline(struct yuv_frame* pdst_frame, struct point_xy dst_point,struct yuv_frame*padd_frame,int line)
{
	if (line%2==0)
	{
		return;
	}

	unsigned char*pdsty, *pdstv,*p_curdstv;
	int dst_w,dst_h;
	dst_w = pdst_frame->width;
	dst_h = pdst_frame->height;
	pdsty = pdst_frame->pdata;
	pdstv = pdst_frame->pdata + dst_w * dst_h;
	p_curdstv = pdstv + (dst_point.h+line)/2*dst_w/2 + dst_point.w/2;

	unsigned char*paddy, *paddv,*p_curaddv;
	int add_w,add_h;
	add_w = padd_frame->width;
	add_h = padd_frame->height;
	paddy = padd_frame->pdata;
	paddv = padd_frame->pdata + add_w * add_h;
	p_curaddv = paddv + line/2*add_w/2;

	int cp_size;
	cp_size = add_w/2;
	memcpy(p_curdstv,p_curaddv,cp_size);
}

void yuv420_add(struct yuv_frame* pdst_frame, struct point_xy dst_point,struct yuv_frame*padd_frame)
{
	int dst_w,dst_h;
	dst_w = pdst_frame->width;
	dst_h = pdst_frame->height;

	int add_w,add_h;
	add_w = padd_frame->width;
	add_h = padd_frame->height;


	dst_point.w = dst_point.w >> 1 <<1;
	dst_point.h = dst_point.h >> 1 <<1;

	if (dst_w<dst_point.w+add_w)
	{
		return;
	}

	if (dst_h<dst_point.h+add_h)
	{
		return;
	}

	for (int i = 0; i < add_h; i++)
	{
		cpy_cur_yline(pdst_frame, dst_point,padd_frame,i);
		cpy_cur_uline(pdst_frame, dst_point,padd_frame,i);
		cpy_cur_vline(pdst_frame, dst_point,padd_frame,i);
	}
}
