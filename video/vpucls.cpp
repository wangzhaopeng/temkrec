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
#include <vector>
extern "C"
{
#include "vpu_lib.h"
#include "vpu_io.h"
};
#include "vpucls.h"

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


static framebuf* AllocFrameBuf(int weight, int height){
	framebuf *fb = new framebuf;
	if (fb == NULL)
		return NULL;
	memset(&(fb->desc), 0, sizeof(vpu_mem_desc));
	fb->desc.size = (weight * height  + weight / 2 * height / 2 * 2);
	int err = IOGetPhyMem(&fb->desc);
	if (err){
		delete fb;fb = NULL;
		return NULL;
	}

	fb->addrY = fb->desc.phy_addr;
	fb->addrCb = fb->addrY + weight * height;
	fb->addrCr = fb->addrCb + weight / 2 * height / 2;
	fb->strideY = weight;
	fb->strideC =  weight / 2;

	fb->desc.virt_uaddr = IOGetVirtMem(&(fb->desc));
	if (fb->desc.virt_uaddr <= 0){
		IOFreePhyMem(&fb->desc);
		delete fb;fb = NULL;
		return NULL;
	}
	return fb;
}

static void FreeFrameBuf(framebuf *fb){
	if (fb == NULL)
		return;
	if (fb->desc.virt_uaddr) 
		IOFreeVirtMem(&fb->desc);
	if (fb->desc.phy_addr)
		IOFreePhyMem(&fb->desc);
	delete fb;fb = NULL;
//cout<<"FreeFrameBuf"<<endl;
}


vpucls::vpucls(const int w,const int h,const int f){
	m_w = w;
	m_h = h;
	m_f = f;
}

vpucls::~vpucls(void){
	de_init();
}

void vpucls::de_init(void){

	FreeEncoderFrameBuffer();

	if(m_virt_addr){
		IOFreeVirtMem((vpu_mem_desc *)mp_vpu_mem_desc);
	}


	if(mp_EncHandle){
		EncHandle *p_EncHandle = (EncHandle *)mp_EncHandle;		
		RetCode ret = vpu_EncClose(*p_EncHandle);
		if (ret == RETCODE_FRAME_NOT_COMPLETE){
			vpu_SWReset(*p_EncHandle, 0);
			vpu_EncClose(*p_EncHandle);
		}
		delete (EncHandle *)mp_EncHandle;
	}

	if(mp_vpu_mem_desc){
		IOFreePhyMem((vpu_mem_desc *)mp_vpu_mem_desc);
		delete (vpu_mem_desc *)mp_vpu_mem_desc;mp_vpu_mem_desc = NULL;
	}

	vector<unsigned char>vtem;
	m_sps=m_pps=vtem;

	mb_IFrame_flag = false;
//vpu_UnInit();
}

////return -1 err    0 ok
int vpucls::init(void){

	vpu_Init(NULL);

	mb_IFrame_flag = false;
	mp_vpu_mem_desc = NULL;
	mp_EncHandle = NULL;
	m_virt_addr = 0;

	mp_vpu_mem_desc = new vpu_mem_desc;
	vpu_mem_desc *p_mem_desc = (vpu_mem_desc *)mp_vpu_mem_desc;
	memset(p_mem_desc,0,sizeof(vpu_mem_desc));

	p_mem_desc->size = STREAM_BUF_SIZE;
	if (IOGetPhyMem(p_mem_desc)){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" IOGetPhyMem err"<<endl;
		delete (vpu_mem_desc *)p_mem_desc;p_mem_desc = NULL;
		return -1;
	}
	m_phy_addr = p_mem_desc->phy_addr;

	EncOpenParam encop = {0};
	encop.bitstreamBuffer = m_phy_addr;
	encop.bitstreamBufferSize = STREAM_BUF_SIZE;
	encop.bitstreamFormat = STD_AVC;
	encop.mapType = 0;
	encop.linear2TiledEnable = 0;
	encop.picWidth = m_w;
	encop.picHeight = m_h;
	encop.frameRateInfo = m_f;
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


	mp_EncHandle = new EncHandle;
	EncHandle *p_EncHandle = (EncHandle *)mp_EncHandle;
	memset(p_EncHandle,0,sizeof(EncHandle));

	RetCode ret = vpu_EncOpen(p_EncHandle, &encop);
	if (ret != RETCODE_SUCCESS){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" vpu_EncOpen err"<<endl;
		delete (EncHandle *)p_EncHandle;p_EncHandle = NULL;
		return -1;
	}
	EncInitialInfo initinfo = {0};
	ret = vpu_EncGetInitialInfo(*p_EncHandle, &initinfo);
	if (ret != RETCODE_SUCCESS) {
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" vpu_EncGetInitialInfo err"<<endl;
		return -1;
	}

	m_virt_addr = IOGetVirtMem(p_mem_desc);

	int nRet = AllocEncoderFrameBuffer();
	if(nRet<0){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" AllocEncoderFrameBuffer err"<<endl;
		return -1;
	}

	return 0;
}

void vpucls::FreeEncoderFrameBuffer(void)
{
	for (int i = 0; i<4+1; i++) {
		FreeFrameBuf((framebuf *)m_pFrameBufferPool[i]);
	}
	free(m_pFrameBuffer);
	free(m_pFrameBufferPool);
}



////return -1 err    0 ok
int vpucls::AllocEncoderFrameBuffer(void){
	EncHandle handle = *(EncHandle*)mp_EncHandle;
	FrameBuffer *fb;
	framebuf **pfbpool;
	unsigned long subSampBaseA = 0, subSampBaseB = 0;
	EncExtBufInfo extbufinfo = {0};
	RetCode ret;
	m_pFrameBuffer = (FrameBuffer*)calloc(5, sizeof(FrameBuffer));
	fb = (FrameBuffer*)m_pFrameBuffer;
	if (fb == NULL) 
		return -1;

	m_pFrameBufferPool = (void**)calloc(5,sizeof(struct frame_buf *));
	pfbpool = (framebuf**)m_pFrameBufferPool;
	if (pfbpool == NULL) 
		return -1;

	for (int i = 0; i < 4+1; i++){
		pfbpool[i] = AllocFrameBuf(m_w, m_h);
		if (pfbpool[i] == NULL)
			return -1;
	}
	for (int i = 0; i < 4+1; i++){
		fb[i].myIndex = i;
		fb[i].bufY = pfbpool[i]->addrY;
		fb[i].bufCb = pfbpool[i]->addrCb;
		fb[i].bufCr = pfbpool[i]->addrCr;
		fb[i].strideY = pfbpool[i]->strideY;
		fb[i].strideC = pfbpool[i]->strideC;
	}
	subSampBaseA = fb[2].bufY;
	subSampBaseB = fb[3].bufY;
	//extbufinfo.scratchBuf = m_struExtBufCfg;
	ret = vpu_EncRegisterFrameBuffer(handle, fb, 2, m_w, m_w,
		subSampBaseA, subSampBaseB, &extbufinfo);
	if (ret != RETCODE_SUCCESS) 
		return -1;

	return 0;
}


////return -1 err    0 ok
int vpucls::get_sps(vector<unsigned char>&sps){
	if(m_sps.size()>0){
		sps = m_sps;
		return 0;
	}
	EncHeaderParam enchdr_param = {0};
	enchdr_param.headerType = SPS_RBSP;
	RetCode ret = vpu_EncGiveCommand(*(EncHandle*)mp_EncHandle, ENC_PUT_AVC_HEADER, &enchdr_param);
	if(ret != RETCODE_SUCCESS){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" vpu_EncGiveCommand err"<<endl;
		return -1;
	}

	vector<unsigned char>v_temsps(enchdr_param.size);

	unsigned long nVirtualaddr = m_virt_addr + (enchdr_param.buf - m_phy_addr);
	memcpy(&v_temsps[0],(char*)nVirtualaddr,enchdr_param.size);
	sps=m_sps=v_temsps;
	return 0;
}

////return -1 err    0 ok
int vpucls::get_pps(vector<unsigned char>&pps){
	if(m_pps.size()>0){
		pps = m_pps;
		return 0;
	}
	EncHeaderParam enchdr_param = {0};
	enchdr_param.headerType = PPS_RBSP;
	RetCode ret = vpu_EncGiveCommand(*(EncHandle*)mp_EncHandle, ENC_PUT_AVC_HEADER, &enchdr_param);
	if(ret != RETCODE_SUCCESS){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" vpu_EncGiveCommand err"<<endl;
		return -1;
	}

	vector<unsigned char>v_tempps(enchdr_param.size);
	unsigned long nVirtualaddr = m_virt_addr + (enchdr_param.buf - m_phy_addr);
	memcpy(&v_tempps[0],(char*)nVirtualaddr,enchdr_param.size);
	pps=m_pps=v_tempps;
	return 0;
}


////return -1 err    0 ok
int vpucls::enc(const void* pYUV420Data,int len,bool bIFrame,std::vector<unsigned char>&v_h264){
	if(!mb_IFrame_flag){
		mb_IFrame_flag = true;
		bIFrame = true;
	}

	int nImageLen = m_w * m_h * 3 / 2;
	if(pYUV420Data == NULL || len != nImageLen)
		return -1;
	EncParam  enc_param = {0};
	FrameBuffer * p_framebuffer = (FrameBuffer *)m_pFrameBuffer;
	enc_param.sourceFrame = &p_framebuffer[4];
	enc_param.quantParam = 23;
	if(bIFrame)
		enc_param.forceIPicture = 1;
	else
		enc_param.forceIPicture = 0;
	enc_param.skipPicture = 0;
	enc_param.enableAutoSkip = 1;
	enc_param.encLeftOffset = 0;
	enc_param.encTopOffset = 0;
	framebuf *pfb = (framebuf *)m_pFrameBufferPool[4];
	unsigned long y_addr = pfb->addrY + pfb->desc.virt_uaddr - pfb->desc.phy_addr;
	memcpy((char*)y_addr,pYUV420Data,len);
	RetCode ret = vpu_EncStartOneFrame(*(EncHandle*)mp_EncHandle, &enc_param);
	if ( ret != RETCODE_SUCCESS){ 
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" vpu_EncStartOneFrame err"<<" ret: "<<ret<<endl;
		return -1;
	}
	int nLoop = 0;
	while (vpu_IsBusy()) {
		vpu_WaitForInt(200);
		if (nLoop == 20){
			cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<" vpu_WaitForInt(200)*20"<<endl;
			cout << "err vpu_SWReset "<<endl;
			ret = vpu_SWReset(*(EncHandle*)mp_EncHandle, 0);
			return -1;
		}
		nLoop++;
	}
	EncOutputInfo outinfo = {0};
	ret = vpu_EncGetOutputInfo(*(EncHandle*)mp_EncHandle, &outinfo);
	if (ret != RETCODE_SUCCESS){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<endl;
		cout << "err vpu_EncGetOutputInfo "<< " ret != RETCODE_SUCCESS "<<"ret "<<ret<<endl;
		return -1;
	}

	unsigned long nVirtualBufferAddr = m_virt_addr + (outinfo.bitstreamBuffer - m_phy_addr);

	v_h264.clear();
//cout << "v_h264 size "<<v_h264.size();
	v_h264.insert(v_h264.begin(),(unsigned char*)nVirtualBufferAddr,(unsigned char*)nVirtualBufferAddr+outinfo.bitstreamSize);
//cout << " v_h264 size2 "<<v_h264.size()<<endl;

	return 0;
}




#include <fstream>


void test_vpu3(void){
	//int w = 1920,h=1080,f=30;
	int w = 1920,h=1080,f=15;

	vector<unsigned char> v_sps,v_pps;

	int iret;
	vpucls vpu_enc(w,h,f);
	vpu_enc.init();

#if 0
{
	for(int i = 0; i< 250; i++){
		vpu_enc.de_init();
		vpu_enc.init();
		cout<<"................"<<i<<endl;
	}
	cout<<"sleep"<<endl;
	sleep(2);
}
#endif

	iret = vpu_enc.get_sps(v_sps);
	iret = vpu_enc.get_pps(v_pps);

	vector<unsigned char> v_0(w*h*3,0),v_f(w*h*3,0xff),v_h264(w*h*3);
	int h264len;

	time_t sec = time(NULL);
	cout <<ctime(&sec);
	ofstream outh264("vpu19.264", ios::out | ios::binary);
	outh264.write((char*)&v_sps[0], v_sps.size()); outh264.write((char*)&v_pps[0], v_pps.size());

//while(1)
	for(int sec = 0; sec < 20; sec++){
		bool bIFrame = true;
		for(int i = 0; i<f;i++){
			vpu_enc.enc(&v_0[0],w*h*3/2,bIFrame,v_h264);
			bIFrame = false;
			outh264.write((char*)&v_h264[0], v_h264.size());
		}

		bIFrame = true;
		for(int i = 0; i < f; i++){
			vpu_enc.enc(&v_f[0],w*h*3/2,bIFrame,v_h264);
			bIFrame = false;
			outh264.write((char*)&v_h264[0], v_h264.size());
		}
		cout << sec<<endl;
	}
	sec = time(NULL);
	cout <<ctime(&sec);

	//vpu_enc.de_init();
}


