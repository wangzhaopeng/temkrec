

#include "stdio.h"
#include "string.h"

#include "yuv2rgb.h"




#define SAT(c) \
        if (c & (~255)) { if (c < 0) c = 0; else c = 255; }
void uyvy_to_rgb24 (int width, int height, unsigned char *src, unsigned char *dst)
{
   unsigned char *s;
   unsigned char *d;
   int l, c;
   int r, g, b, cr, cg, cb, y1, y2;

   l = height;
   s = src;
   d = dst;
   while (l--) {
      c = width >> 1;
      while (c--) {
         cb = ((*s - 128) * 454) >> 8;
         cg = (*s++ - 128) * 88;
         y1 = *s++;
         cr = ((*s - 128) * 359) >> 8;
         cg = (cg + (*s++ - 128) * 183) >> 8;
         y2 = *s++;

         r = y1 + cr;
         b = y1 + cb;
         g = y1 - cg;
         SAT(r);
         SAT(g);
         SAT(b);

     *d++ = b;
     *d++ = g;
     *d++ = r;

         r = y2 + cr;
         b = y2 + cb;
         g = y2 - cg;
         SAT(r);
         SAT(g);
         SAT(b);

     *d++ = b;
     *d++ = g;
     *d++ = r;
      }
   }
}

void yuyv_to_rgb24 (int width, int height, unsigned char *src, unsigned char *dst)
{
	unsigned char *s;
	unsigned char *d;
	int l, c;
	int r, g, b, cr, cg, cb, y1, y2;

	l = height;
	s = src;
	d = dst;
	while (l--) {
		c = width >> 1;
		while (c--) {
			y1 = *s++;
			cb = ((*s - 128) * 454) >> 8;
			cg = (*s++ - 128) * 88;
			y2 = *s++;
			cr = ((*s - 128) * 359) >> 8;
			cg = (cg + (*s++ - 128) * 183) >> 8;

			r = y1 + cr;
			b = y1 + cb;
			g = y1 - cg;
			SAT(r);
			SAT(g);
			SAT(b);

			*d++ = b;
			*d++ = g;
			*d++ = r;

			r = y2 + cr;
			b = y2 + cb;
			g = y2 - cg;
			SAT(r);
			SAT(g);
			SAT(b);

			*d++ = b;
			*d++ = g;
			*d++ = r;

		}
	}
}

#define LIMIT(x) ((x)>0xffffff?0xff: ((x)<=0xffff?0:((x)>>16)))

static inline void
move_420_block(int yTL, int yTR, int yBL, int yBR, int u, int v,
               int rowPixels, unsigned char * rgb)
{
    const int rvScale = 91881;
    const int guScale = -22553;
    const int gvScale = -46801;
    const int buScale = 116129;
    const int yScale  = 65536;
    int r, g, b;

    g = guScale * u + gvScale * v;
//  if (force_rgb) {
//      r = buScale * u;
//      b = rvScale * v;
//  } else {
        r = rvScale * v;
        b = buScale * u;
//  }

    yTL *= yScale; yTR *= yScale;
    yBL *= yScale; yBR *= yScale;

    /* Write out top two pixels */
    rgb[0] = LIMIT(b+yTL); rgb[1] = LIMIT(g+yTL);
    rgb[2] = LIMIT(r+yTL);

    rgb[3] = LIMIT(b+yTR); rgb[4] = LIMIT(g+yTR);
    rgb[5] = LIMIT(r+yTR);

    /* Skip down to next line to write out bottom two pixels */
    rgb += 3 * rowPixels;
    rgb[0] = LIMIT(b+yBL); rgb[1] = LIMIT(g+yBL);
    rgb[2] = LIMIT(r+yBL);

    rgb[3] = LIMIT(b+yBR); rgb[4] = LIMIT(g+yBR);
    rgb[5] = LIMIT(r+yBR);
}

#if 0
void yuv420_to_rgb24(int width, int height,
        unsigned char *pIn0, unsigned char *pOut0)
{
    const int bytes = 24 >> 3;
    int i, j, y00, y01, y10, y11, u, v;
    unsigned char *pY = pIn0;
    unsigned char *pU = pY + 4;
    unsigned char *pV = pU + width;
    unsigned char *pOut = pOut0;

    for (j = 0; j <= height - 2; j += 2) {
        for (i = 0; i <= width - 4; i += 4) {
            y00 = *pY;
            y01 = *(pY + 1);
            y10 = *(pY + width);
            y11 = *(pY + width + 1);
            u = (*pU++) - 128;
            v = (*pV++) - 128;

            move_420_block(y00, y01, y10, y11, u, v,
                       width, pOut);

            pY += 2;
            pOut += 2 * bytes;

            y00 = *pY;
            y01 = *(pY + 1);
            y10 = *(pY + width);
            y11 = *(pY + width + 1);
            u = (*pU++) - 128;
            v = (*pV++) - 128;

            move_420_block(y00, y01, y10, y11, u, v,
                       width, pOut);

            pY += 4; // skip UV
            pOut += 2 * bytes;

        }
        pY += width;
        pOut += width * bytes;
    }
}
#endif


#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))
bool  RGB24toYUV420(unsigned char*  RgbBuf,int nWidth,int nHeight,unsigned char* yuvBuf)  
{  
	int i, j;  
	unsigned char*bufY, *bufU, *bufV, *bufRGB,*bufYuv;  
	unsigned long len_v;
	unsigned long *len=&len_v;
	*len = nWidth * nHeight+(nWidth * nHeight)/2;
	memset(yuvBuf,0,(unsigned int )*len);  
	bufY = yuvBuf;  
	bufV = yuvBuf + nWidth * nHeight;  
	bufU = bufV + (nWidth * nHeight* 1/4);  
	*len = 0;   
	unsigned char y, u, v, r, g, b,testu,testv;  
	unsigned int ylen = nWidth * nHeight;  
	unsigned int ulen = (nWidth * nHeight)/4;  
	unsigned int vlen = (nWidth * nHeight)/4;   
	for (j = 0; j<nHeight;j++)  
	{  
		//bufRGB = RgbBuf + nWidth * (nHeight - 1 - j) * 3 ;  
		bufRGB = RgbBuf + nWidth * (j) * 3 ;  
		for (i = 0;i<nWidth;i++)  
		{  
			//int pos = nWidth * i + j;  
			r = *(bufRGB++);  
			g = *(bufRGB++);  
			b = *(bufRGB++);  
			y = (unsigned char)( ( 66 * r + 129 * g +  25 * b + 128) >> 8) + 16  ;            
			u = (unsigned char)( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128 ;            
			v = (unsigned char)( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128 ;  
			*(bufY++) = max( 0, min(y, 255 ));  
			if (j%2==0)  
			{  
				if (i%2==0)  
				{ 
				if (u>255)  
				{  
					u=255;  
				}  
				if (u<0)  
				{  
					u = 0;  
				}  
				*(bufU++) =u;  
				//存u分量  
				}
			}  
			else  
			{  
				//存v分量  
				if (i%2==0)  
				{  
					if (v>255)  
					{  
						v = 255;  
					}  
					if (v<0)  
					{  
						v = 0;  
					}  
					*(bufV++) =v;  
				}  
			}  
		}  
	}  
	*len = nWidth * nHeight+(nWidth * nHeight)/2;  
	return true;  
}   


bool YUV420toRGB24(unsigned char* yuvBuf, int nWidth, int nHeight, unsigned char * pRgbBuf)
{
#define PIXELSIZE nWidth * nHeight  
	const int Table_fv1[256] = { -180, -179, -177, -176, -174, -173, -172, -170, -169, -167, -166, -165, -163, -162, -160, -159, -158, -156, -155, -153, -152, -151, -149, -148, -146, -145, -144, -142, -141, -139, -138, -137, -135, -134, -132, -131, -130, -128, -127, -125, -124, -123, -121, -120, -118, -117, -115, -114, -113, -111, -110, -108, -107, -106, -104, -103, -101, -100, -99, -97, -96, -94, -93, -92, -90, -89, -87, -86, -85, -83, -82, -80, -79, -78, -76, -75, -73, -72, -71, -69, -68, -66, -65, -64, -62, -61, -59, -58, -57, -55, -54, -52, -51, -50, -48, -47, -45, -44, -43, -41, -40, -38, -37, -36, -34, -33, -31, -30, -29, -27, -26, -24, -23, -22, -20, -19, -17, -16, -15, -13, -12, -10, -9, -8, -6, -5, -3, -2, 0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14, 15, 16, 18, 19, 21, 22, 23, 25, 26, 28, 29, 30, 32, 33, 35, 36, 37, 39, 40, 42, 43, 44, 46, 47, 49, 50, 51, 53, 54, 56, 57, 58, 60, 61, 63, 64, 65, 67, 68, 70, 71, 72, 74, 75, 77, 78, 79, 81, 82, 84, 85, 86, 88, 89, 91, 92, 93, 95, 96, 98, 99, 100, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120, 122, 123, 124, 126, 127, 129, 130, 131, 133, 134, 136, 137, 138, 140, 141, 143, 144, 145, 147, 148, 150, 151, 152, 154, 155, 157, 158, 159, 161, 162, 164, 165, 166, 168, 169, 171, 172, 173, 175, 176, 178 };
	const int Table_fv2[256] = { -92, -91, -91, -90, -89, -88, -88, -87, -86, -86, -85, -84, -83, -83, -82, -81, -81, -80, -79, -78, -78, -77, -76, -76, -75, -74, -73, -73, -72, -71, -71, -70, -69, -68, -68, -67, -66, -66, -65, -64, -63, -63, -62, -61, -61, -60, -59, -58, -58, -57, -56, -56, -55, -54, -53, -53, -52, -51, -51, -50, -49, -48, -48, -47, -46, -46, -45, -44, -43, -43, -42, -41, -41, -40, -39, -38, -38, -37, -36, -36, -35, -34, -33, -33, -32, -31, -31, -30, -29, -28, -28, -27, -26, -26, -25, -24, -23, -23, -22, -21, -21, -20, -19, -18, -18, -17, -16, -16, -15, -14, -13, -13, -12, -11, -11, -10, -9, -8, -8, -7, -6, -6, -5, -4, -3, -3, -2, -1, 0, 0, 1, 2, 2, 3, 4, 5, 5, 6, 7, 7, 8, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 17, 18, 19, 20, 20, 21, 22, 22, 23, 24, 25, 25, 26, 27, 27, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35, 36, 37, 37, 38, 39, 40, 40, 41, 42, 42, 43, 44, 45, 45, 46, 47, 47, 48, 49, 50, 50, 51, 52, 52, 53, 54, 55, 55, 56, 57, 57, 58, 59, 60, 60, 61, 62, 62, 63, 64, 65, 65, 66, 67, 67, 68, 69, 70, 70, 71, 72, 72, 73, 74, 75, 75, 76, 77, 77, 78, 79, 80, 80, 81, 82, 82, 83, 84, 85, 85, 86, 87, 87, 88, 89, 90, 90 };
	const int Table_fu1[256] = { -44, -44, -44, -43, -43, -43, -42, -42, -42, -41, -41, -41, -40, -40, -40, -39, -39, -39, -38, -38, -38, -37, -37, -37, -36, -36, -36, -35, -35, -35, -34, -34, -33, -33, -33, -32, -32, -32, -31, -31, -31, -30, -30, -30, -29, -29, -29, -28, -28, -28, -27, -27, -27, -26, -26, -26, -25, -25, -25, -24, -24, -24, -23, -23, -22, -22, -22, -21, -21, -21, -20, -20, -20, -19, -19, -19, -18, -18, -18, -17, -17, -17, -16, -16, -16, -15, -15, -15, -14, -14, -14, -13, -13, -13, -12, -12, -11, -11, -11, -10, -10, -10, -9, -9, -9, -8, -8, -8, -7, -7, -7, -6, -6, -6, -5, -5, -5, -4, -4, -4, -3, -3, -3, -2, -2, -2, -1, -1, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 29, 29, 29, 30, 30, 30, 31, 31, 31, 32, 32, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 40, 40, 40, 41, 41, 41, 42, 42, 42, 43, 43 };
	const int Table_fu2[256] = { -227, -226, -224, -222, -220, -219, -217, -215, -213, -212, -210, -208, -206, -204, -203, -201, -199, -197, -196, -194, -192, -190, -188, -187, -185, -183, -181, -180, -178, -176, -174, -173, -171, -169, -167, -165, -164, -162, -160, -158, -157, -155, -153, -151, -149, -148, -146, -144, -142, -141, -139, -137, -135, -134, -132, -130, -128, -126, -125, -123, -121, -119, -118, -116, -114, -112, -110, -109, -107, -105, -103, -102, -100, -98, -96, -94, -93, -91, -89, -87, -86, -84, -82, -80, -79, -77, -75, -73, -71, -70, -68, -66, -64, -63, -61, -59, -57, -55, -54, -52, -50, -48, -47, -45, -43, -41, -40, -38, -36, -34, -32, -31, -29, -27, -25, -24, -22, -20, -18, -16, -15, -13, -11, -9, -8, -6, -4, -2, 0, 1, 3, 5, 7, 8, 10, 12, 14, 15, 17, 19, 21, 23, 24, 26, 28, 30, 31, 33, 35, 37, 39, 40, 42, 44, 46, 47, 49, 51, 53, 54, 56, 58, 60, 62, 63, 65, 67, 69, 70, 72, 74, 76, 78, 79, 81, 83, 85, 86, 88, 90, 92, 93, 95, 97, 99, 101, 102, 104, 106, 108, 109, 111, 113, 115, 117, 118, 120, 122, 124, 125, 127, 129, 131, 133, 134, 136, 138, 140, 141, 143, 145, 147, 148, 150, 152, 154, 156, 157, 159, 161, 163, 164, 166, 168, 170, 172, 173, 175, 177, 179, 180, 182, 184, 186, 187, 189, 191, 193, 195, 196, 198, 200, 202, 203, 205, 207, 209, 211, 212, 214, 216, 218, 219, 221, 223, 225 };
	if (!yuvBuf || !pRgbBuf)
		return false;
	const long nYLen = long(PIXELSIZE);
	const int nHfWidth = (nWidth >> 1);
	if (nYLen<1 || nHfWidth<1)
		return false;
	// Y data  
	unsigned char* yData = yuvBuf;
	// v data  
	unsigned char* vData = &yData[nYLen];
	// u data  
	unsigned char* uData = &vData[nYLen >> 2];
	if (!uData || !vData)
		return false;
	int rgb[3];
	int i, j, m, n, x, y, py, rdif, invgdif, bdif;
	m = -nWidth;
	n = -nHfWidth;
	bool addhalf = true;
	for (y = 0; y<nHeight; y++) {
		m += nWidth;
		if (addhalf){
			n += nHfWidth;
			addhalf = false;
		}
		else {
			addhalf = true;
		}
		for (x = 0; x<nWidth; x++)  {
			i = m + x;
			j = n + (x >> 1);
			py = yData[i];
			// search tables to get rdif invgdif and bidif  
			rdif = Table_fv1[vData[j]];    // fv1  
			invgdif = Table_fu1[uData[j]] + Table_fv2[vData[j]]; // fu1+fv2  
			bdif = Table_fu2[uData[j]]; // fu2       
			rgb[0] = py + rdif;    // R  
			rgb[1] = py - invgdif; // G  
			rgb[2] = py + bdif;    // B  
			j = nYLen - nWidth - m + x;
			i = (j << 1) + j;
			// copy this pixel to rgb data  
			for (j = 0; j<3; j++)
			{
				if (rgb[j] >= 0 && rgb[j] <= 255){
					pRgbBuf[i + j] = rgb[j];
				}
				else{
					pRgbBuf[i + j] = (rgb[j] < 0) ? 0 : 255;
				}
			}
		}
	}
	return true;
}



void yuv_to_yuv420 (int width, int height, unsigned char *src, unsigned char *dst)
{

	{///////part1
		int i = 0;
		i = 0;
		int con_v;
		con_v = width*height;
		for(int i = 0; i< con_v;i++)
		{
			dst[i] = src[i*2];
		}
	}


	{///////part2
		int line = 0;
		int i = width*height;
		unsigned char *src_yuv;
		for(line = 0; line < height/4*2; line++)
		{
			src_yuv = &src[line*width*4+width*2];
			int con_v;
			con_v = width/2;
			for(int j = 0; j < con_v; j++,i++)
			{
				dst[i] = src_yuv[j*4+1];
			}
	
		}
	}

	{///////part3
		int line = 0;
		int i = width*height+width*height/4;
		unsigned char *src_yuv;
		for(line = 0; line < height/4*2; line++)
		{
			src_yuv = &src[line*width*4+width*2];
			int con_v;
			con_v = width/2;
			for(int j = 0; j < con_v; j++,i++)
			{
				dst[i] = src_yuv[j*4+3];
			}
	
		}
	}


#if 0
{
FILE *fp = fopen("s1420y","wb");
fwrite(dst,1,width*height*3/2,fp);
fclose(fp);
}
#endif

}



