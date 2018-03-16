
#include "stdio.h"


#include "rgb_bmp.h"

#include "stdlib.h"
#include "string.h"


#pragma pack(1)
typedef struct tagBITMAPFILEHEADER
{
	unsigned short bfType;
	unsigned long bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long bfOffBits;

} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
	unsigned long biSize;
	unsigned long biWidth;
	unsigned long biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long biCompression;
	unsigned long biSizeImage;
	unsigned long biXPelsPerMeter;
	unsigned long biYPelsPerMeter;
	unsigned long biClrUsed;
	unsigned long biClrImportant;
} BITMAPINFOHEADER;
#pragma pack()

static void rgb24_rev(unsigned char *pd,int width,int height)
{
	int rgb_size = width * height *3;
	unsigned char *p_tem = (unsigned char*)malloc(rgb_size);
	for(int i = 0; i < height; i++){
		memcpy(p_tem+i*width*3,pd+(height-1-i)*width*3,width*3);
	}
	memcpy(pd,p_tem,width*height*3);
	free(p_tem);
}

static void rgb24_mirror(unsigned char *pd,int width,int height)
{
	int rgb_size = width * height *3;
	unsigned char *p_tem = (unsigned char*)malloc(rgb_size);
	for(int i = 0; i < height; i++){
		for(int j = 0; j < width; j++){
			*(p_tem+i*width*3+j*3) = *(pd+i*width*3+ (width-1-j)*3);
			*(p_tem+i*width*3+j*3 +1) = *(pd+i*width*3+ (width-1-j)*3 +1);
			*(p_tem+i*width*3+j*3 +2) = *(pd+i*width*3+ (width-1-j)*3 +2);
		}
	}
	memcpy(pd,p_tem,width*height*3);
	free(p_tem);
}


int save_rgb24_bmp(unsigned char* pd, char * file_name, int width, int height)
{
	FILE *fpW = NULL;
	BITMAPFILEHEADER bmpFHeader;
	BITMAPINFOHEADER bmpInfo;
	fpW = fopen(file_name, "wb");
	if(fpW == NULL)
	{
		return -1;
	}

	bmpFHeader.bfType = 0x4D42; //16进制
	//  bmpFHeader.bfType=19778;
	bmpFHeader.bfSize = 54L + width * height * 3;
	bmpFHeader.bfReserved1 = 0;
	bmpFHeader.bfReserved2 = 0;
	bmpFHeader.bfOffBits = 54L;

	bmpInfo.biSize = 40L;
	bmpInfo.biWidth = width;
	bmpInfo.biHeight = height;
	bmpInfo.biPlanes = 1;
	bmpInfo.biBitCount = 24;
	bmpInfo.biCompression = 0;
	bmpInfo.biSizeImage = width * height * 3;
	bmpInfo.biXPelsPerMeter = 0;
	bmpInfo.biYPelsPerMeter = 0;
	bmpInfo.biClrUsed = 256;
	bmpInfo.biClrImportant = 256;
	fwrite(&bmpFHeader, sizeof(BITMAPFILEHEADER), 1, fpW);
	fwrite(&bmpInfo, sizeof(BITMAPINFOHEADER), 1, fpW);
	fseek(fpW, 54L, SEEK_SET);
#if 0
	for(int i = 0; i < height; i++)
	{
		for(int j = 0; j < width; j++)
		{
			unsigned char val;
			unsigned char *addr;
			addr = pd + (width*i+j)*3;
			val = *(addr+0);
			*(addr+0) = *(addr+2);
			*(addr+2) = val;
		}
	}
#endif
	//rgb24_rev(pd,width,height);
	//rgb24_mirror(pd, width,height);
	fwrite(pd,1,width * height * 3,fpW);
	fclose(fpW);
	return 0;
}



