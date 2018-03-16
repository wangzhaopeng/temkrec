#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <iostream>
extern "C"
{
#include "vpu_lib.h"
#include "vpu_io.h"
};
#include "VPUWraper.h"

using namespace std;

#define STREAM_BUF_SIZE		0x200000

class framebuf
{
public:
	int addrY;
	int addrCb;
	int addrCr;
	int strideY;
	int strideC;
	int mvColBuf;
	vpu_mem_desc desc;
};

class Encoder
{
public:
	EncHandle m_nHandle;
	unsigned long m_nPhyAddr;
	unsigned long m_nVirtualAddr;
	int m_nYUV420Widht;      
	int m_nYUV420Height;      
	FrameBuffer *m_pFrameBuffer;
	framebuf **m_pFrameBufferPool;
	ExtBufCfg m_struExtBufCfg;
	int m_ndstfg;
};

static framebuf* AllocFrameBuf(int weight, int height)
{
	framebuf *fb = new framebuf;
	if (fb == NULL)
		return NULL;
	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
	fb->desc.size = (weight * height  + weight / 2 * height / 2 * 2);
	int err = IOGetPhyMem(&fb->desc);
	if (err) 
	{
		delete fb;fb = NULL;
		return NULL;
	}

	fb->addrY = fb->desc.phy_addr;
	fb->addrCb = fb->addrY + weight * height;
	fb->addrCr = fb->addrCb + weight / 2 * height / 2;
	fb->strideY = weight;
	fb->strideC =  weight / 2;

	fb->desc.virt_uaddr = IOGetVirtMem(&(fb->desc));
	if (fb->desc.virt_uaddr <= 0)
	{
		IOFreePhyMem(&fb->desc);
		delete fb;fb = NULL;
		return NULL;
	}
	return fb;
}

static void FreeFrameBuf(framebuf *fb)
{
	if (fb == NULL)
		return;
	if (fb->desc.virt_uaddr) 
		IOFreeVirtMem(&fb->desc);
	if (fb->desc.phy_addr)
		IOFreePhyMem(&fb->desc);
	delete fb;fb = NULL;
}

static int AllocEncoderFrameBuffer(Encoder *pEncoder)
{
	EncHandle handle = pEncoder->m_nHandle;
	FrameBuffer *fb;
	framebuf **pfbpool;
	unsigned long subSampBaseA = 0, subSampBaseB = 0;
	EncExtBufInfo extbufinfo = {0};
	RetCode ret;
	fb = pEncoder->m_pFrameBuffer = (FrameBuffer*)calloc(5, sizeof(FrameBuffer));
	if (fb == NULL) 
		return -1;

	pfbpool = pEncoder->m_pFrameBufferPool = (framebuf**)calloc(5,sizeof(struct frame_buf *));
	if (pfbpool == NULL) 
		return -1;

	for (int i = 0; i < 4; i++) 
	{
		pfbpool[i] = AllocFrameBuf(pEncoder->m_nYUV420Widht, pEncoder->m_nYUV420Height);
		if (pfbpool[i] == NULL)
			return -1;
	}
	for (int i = 0; i < 4; i++) 
	{
		fb[i].myIndex = i;
		fb[i].bufY = pfbpool[i]->addrY;
		fb[i].bufCb = pfbpool[i]->addrCb;
		fb[i].bufCr = pfbpool[i]->addrCr;
		fb[i].strideY = pfbpool[i]->strideY;
		fb[i].strideC = pfbpool[i]->strideC;
	}
	subSampBaseA = fb[2].bufY;
	subSampBaseB = fb[3].bufY;
	extbufinfo.scratchBuf = pEncoder->m_struExtBufCfg;
	ret = vpu_EncRegisterFrameBuffer(handle, fb, 2, pEncoder->m_nYUV420Widht, pEncoder->m_nYUV420Widht,
		subSampBaseA, subSampBaseB, &extbufinfo);
	if (ret != RETCODE_SUCCESS) 
		return -1;
	pfbpool[4] = AllocFrameBuf(pEncoder->m_nYUV420Widht, pEncoder->m_nYUV420Height);
	if (pfbpool[4] == NULL) 
		return -1;
	fb[4].myIndex = 4;
	fb[4].bufY = pfbpool[4]->addrY;
	fb[4].bufCb = pfbpool[4]->addrCb;
	fb[4].bufCr = pfbpool[4]->addrCr;
	fb[4].strideY = pfbpool[4]->strideY;
	fb[4].strideC = pfbpool[4]->strideC;
	return 0;
}

static void FreeEncoderFrameBuffer(Encoder *pEncoder)
{
	for (int i = 0; i<4; i++) 
	{
		FreeFrameBuf(pEncoder->m_pFrameBufferPool[i]);
	}
	free(pEncoder->m_pFrameBuffer);
	free(pEncoder->m_pFrameBufferPool);
}

int VPUWraper::Init()
{
        //printf("******************%s,%s,%d\r\n",__FILE__,__func__,__LINE__);
	vpu_Init(NULL);
        //printf("*******************%s,%s,%d\r\n",__FILE__,__func__,__LINE__);
	return 0;
}

int VPUWraper::Fini()
{
	vpu_UnInit();
	return 0;
}

int VPUWraper::OpenEncoder(int nYUV420Width,int nYUV420Height,int frameRate)
{
	vpu_mem_desc	mem_desc = {0};
	mem_desc.size = STREAM_BUF_SIZE;
	if (IOGetPhyMem(&mem_desc)) 
		return (int)NULL;
	EncOpenParam encop = {0};
	encop.bitstreamBuffer = mem_desc.phy_addr;
	encop.bitstreamBufferSize = STREAM_BUF_SIZE;
	encop.bitstreamFormat = STD_AVC;
	encop.mapType = 0;
	encop.linear2TiledEnable = 0;
	encop.picWidth = nYUV420Width;
	encop.picHeight = nYUV420Height;
	encop.frameRateInfo = frameRate;
	encop.bitRate = 0;
	encop.gopSize = 0;
	encop.slicemode.sliceMode = 0;	          /* 0: 1 slice per picture; 1: Multiple slices per picture */
	encop.slicemode.sliceSizeMode = 0;        /* 0: silceSize defined by bits; 1: sliceSize defined by MB number*/
	encop.slicemode.sliceSize = 4000;         /* Size of a slice in bits or MB numbers */
	encop.initialDelay = 0;
	encop.vbvBufferSize = 0;                  /* 0 = ignore 8 */
	encop.intraRefresh = 0;
	encop.sliceReport = 0;
	encop.mbReport = 0;
	encop.mbQpReport = 0;
	encop.rcIntraQp = -1;
	encop.userQpMax = 0;
	encop.userQpMin = 0;
	encop.userQpMinEnable = 0;
	encop.userQpMaxEnable = 0;
	encop.IntraCostWeight = 0;
	encop.MEUseZeroPmv  = 0;
	encop.MESearchRange = 3;                    /* (3: 16x16, 2:32x16, 1:64x32, 0:128x64, H.263(Short Header : always 3) */
	encop.userGamma = (Uint32)(0.75*32768);     /*  (0*32768 <= gamma <= 1*32768) */
	encop.RcIntervalMode= 1;                    /* 0:normal, 1:frame_level, 2:slice_level, 3: user defined Mb_level */
	encop.MbInterval = 0;
	encop.avcIntra16x16OnlyModeEnable = 0;
	encop.ringBufferEnable = 0;
	encop.dynamicAllocEnable = 0;
	encop.chromaInterleave = 0;
	encop.EncStdParam.avcParam.avc_constrainedIntraPredFlag = 0;
	encop.EncStdParam.avcParam.avc_disableDeblk = 0;
	encop.EncStdParam.avcParam.avc_deblkFilterOffsetAlpha = 6;
	encop.EncStdParam.avcParam.avc_deblkFilterOffsetBeta = 0;
	encop.EncStdParam.avcParam.avc_chromaQpOffset = 10;
	encop.EncStdParam.avcParam.avc_audEnable = 0;
	encop.EncStdParam.avcParam.interview_en = 0;
	encop.EncStdParam.avcParam.paraset_refresh_en = 0;
	encop.EncStdParam.avcParam.prefix_nal_en = 0;
	encop.EncStdParam.avcParam.mvc_extension = 0;
	encop.EncStdParam.avcParam.avc_frameCroppingFlag = 0;
	encop.EncStdParam.avcParam.avc_frameCropLeft = 0;
	encop.EncStdParam.avcParam.avc_frameCropRight = 0;
	encop.EncStdParam.avcParam.avc_frameCropTop = 0;
	encop.EncStdParam.avcParam.avc_frameCropBottom = 0;

	EncHandle handle = {0};
	RetCode ret = vpu_EncOpen(&handle, &encop);
	if (ret != RETCODE_SUCCESS) 
		return -1;
	EncInitialInfo initinfo = {0};
	ret = vpu_EncGetInitialInfo(handle, &initinfo);
	if (ret != RETCODE_SUCCESS) 
		return -1;
	Encoder *pEncoder = new Encoder;
	if(pEncoder == NULL)
		return -1;
	pEncoder->m_nVirtualAddr = IOGetVirtMem(&mem_desc);
	pEncoder->m_nPhyAddr = mem_desc.phy_addr;
	pEncoder->m_nYUV420Widht = nYUV420Width;
	pEncoder->m_nYUV420Height = nYUV420Height;
	pEncoder->m_nHandle = handle;
	int nRet = AllocEncoderFrameBuffer(pEncoder);
	if(nRet<0)
		return nRet;
	m_lHandle = (long)pEncoder;
	return (unsigned long)pEncoder;
}

int VPUWraper::EncoderGetSPS(char* szSPS,int &nlen)
{
	Encoder* pEncoder = (Encoder*)m_lHandle;
	if(pEncoder == NULL)
		return -1;
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = pEncoder->m_nHandle;
	enchdr_param.headerType = SPS_RBSP;
	RetCode ret = vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
	if(ret != RETCODE_SUCCESS && nlen>enchdr_param.size)
		return ret;
	unsigned long nVirtualaddr = pEncoder->m_nVirtualAddr + (enchdr_param.buf - pEncoder->m_nPhyAddr);
	memcpy(szSPS,(char*)nVirtualaddr,enchdr_param.size);
	nlen = enchdr_param.size;
	return 0;
}

int VPUWraper::EncoderGetPPS(char* szPPS,int &nlen)
{
	Encoder* pEncoder = (Encoder*)m_lHandle;
	if(pEncoder == NULL)
		return -1;
	EncHeaderParam enchdr_param = {0};
	EncHandle handle = pEncoder->m_nHandle;
	enchdr_param.headerType = PPS_RBSP;
	RetCode ret = vpu_EncGiveCommand(handle, ENC_PUT_AVC_HEADER, &enchdr_param);
	if(ret != RETCODE_SUCCESS && nlen>enchdr_param.size)
		return ret;
	unsigned long nVirtualaddr = pEncoder->m_nVirtualAddr + (enchdr_param.buf - pEncoder->m_nPhyAddr);
	memcpy(szPPS,(char*)nVirtualaddr,enchdr_param.size);
	nlen = enchdr_param.size;
	return 0;
}

int VPUWraper::EncoderOneFrame(const char* szYUV420Data,int nlen,bool bIFrame,char* szH264Data,int &nH264Datalen)
{
	Encoder* pEncoder = (Encoder*)m_lHandle;
	if(pEncoder == NULL)
		return -1;
	int nImageLen = pEncoder->m_nYUV420Widht * pEncoder->m_nYUV420Height * 3 / 2;
	if(szYUV420Data == NULL || nlen != nImageLen)
		return -1;
	EncParam  enc_param = {0};
	enc_param.sourceFrame = &pEncoder->m_pFrameBuffer[4];
	enc_param.quantParam = 23;
	if(bIFrame)
		enc_param.forceIPicture = 1;
	else
		enc_param.forceIPicture = 0;
	enc_param.skipPicture = 0;
	enc_param.enableAutoSkip = 1;
	enc_param.encLeftOffset = 0;
	enc_param.encTopOffset = 0;
	framebuf *pfb = pEncoder->m_pFrameBufferPool[4];
	unsigned long y_addr = pfb->addrY + pfb->desc.virt_uaddr - pfb->desc.phy_addr;
	memcpy((char*)y_addr,szYUV420Data,nlen);
	RetCode ret = vpu_EncStartOneFrame(pEncoder->m_nHandle, &enc_param);
	if ( ret != RETCODE_SUCCESS){ 
		cout << "err vpu_EncStartOneFrame "<< " ret != RETCODE_SUCCESS "<<"ret "<<ret<<endl;
		return ret;
	}
	int nLoop = 0;
	while (vpu_IsBusy()) 
	{
		vpu_WaitForInt(200);
		if (nLoop == 20) 
		{
			cout << "err vpu_SWReset "<<endl;
			ret = vpu_SWReset(pEncoder->m_nHandle, 0);
			return -1;
		}
		nLoop++;
	}
	EncOutputInfo outinfo = {0};
	ret = vpu_EncGetOutputInfo(pEncoder->m_nHandle, &outinfo);
	if (ret != RETCODE_SUCCESS){
		cout << "err vpu_EncGetOutputInfo "<< " ret != RETCODE_SUCCESS "<<"ret "<<ret<<endl;
		return ret;
	}
	unsigned long nVirtualBufferAddr = pEncoder->m_nVirtualAddr + (outinfo.bitstreamBuffer - pEncoder->m_nPhyAddr);
	memcpy(szH264Data,(char*)nVirtualBufferAddr,outinfo.bitstreamSize);
	nH264Datalen = outinfo.bitstreamSize;
	return 0;
}

int VPUWraper::CloseEncoder()
{
	Encoder* pEncoder = (Encoder*)m_lHandle;
	if(pEncoder == NULL)
		return -1;
	FreeEncoderFrameBuffer(pEncoder);
	RetCode ret = vpu_EncClose(pEncoder->m_nHandle);
	if (ret == RETCODE_FRAME_NOT_COMPLETE) 
	{
		vpu_SWReset(pEncoder->m_nHandle, 0);
		vpu_EncClose(pEncoder->m_nHandle);
	}
	free(pEncoder);
	return 0;
}

#include <iostream>
#include <vector>
#include <fstream>
using namespace std;
void test_vpu(void){
//int w = 1920,h=1080,f=30;
int w = 1920,h=1080,f=15;

vector<unsigned char> v_sps(1024);
vector<unsigned char> v_pps(1024);
int spslen,ppslen;

int iret;
VPUWraper::Init();
VPUWraper vpu_enc;
iret = vpu_enc.OpenEncoder(w,h,f);cout << "openEncoder iret:"<<iret<<endl;

iret = vpu_enc.EncoderGetSPS((char*)&v_sps[0],spslen);cout << "EncoderGetSPS iret:"<<iret<<endl;
iret = vpu_enc.EncoderGetPPS((char*)&v_pps[0],ppslen);cout << "EncoderGetPPS iret:"<<iret<<endl;
v_sps.resize(spslen);cout << "spslen:"<<spslen<<endl;
v_pps.resize(ppslen);cout << "ppslen:"<<ppslen<<endl;

vector<unsigned char> v_0(w*h*3,0);
vector<unsigned char> v_f(w*h*3,0xff);

vector<unsigned char> v_h264(w*h*3);
int h264len;

time_t sec = time(NULL);
cout <<ctime(&sec);
ofstream outh264;
outh264.open("vpu.264", ios::out | ios::binary);
outh264.write((char*)&v_sps[0], v_sps.size());
outh264.write((char*)&v_pps[0], v_pps.size());
for(int sec = 0; sec < 10; sec++){
	bool bIFrame = true;
	for(int i = 0; i<f;i++){
		vpu_enc.EncoderOneFrame((char*)&v_0[0],w*h*3/2,bIFrame,(char*)&v_h264[0],h264len);
		bIFrame = false;
		outh264.write((char*)&v_h264[0], h264len);
	}

	bIFrame = true;
	for(int i = 0; i < f; i++){
		vpu_enc.EncoderOneFrame((char*)&v_f[0],w*h*3/2,bIFrame,(char*)&v_h264[0],h264len);
		bIFrame = false;
		outh264.write((char*)&v_h264[0], h264len);
	}
	cout << sec<<endl;
}
sec = time(NULL);
cout <<ctime(&sec);

}


