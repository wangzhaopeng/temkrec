#ifndef __VPU_CLS_H__
#define __VPU_CLS_H__

#include <vector>

class vpucls{
public:
	vpucls(const int w,const int h,const int f=30);
	~vpucls();
	int init(void);
	void de_init(void);
	int get_sps(std::vector<unsigned char>&sps);
	int get_pps(std::vector<unsigned char>&pps);
	int enc(const char* pYUV420Data,int len,bool bIFrame,std::vector<unsigned char>&v_h264);

private:
	int m_w,m_h,m_f;

	std::vector<unsigned char>m_sps,m_pps;
	bool mb_IFrame_flag;

	void*mp_vpu_mem_desc;
	void*mp_EncHandle;
	void **m_pFrameBufferPool;
	void *m_pFrameBuffer;
	unsigned long m_phy_addr;
	unsigned long m_virt_addr;
	//ExtBufCfg m_struExtBufCfg;

	void FreeEncoderFrameBuffer(void);
	int AllocEncoderFrameBuffer(void);
};


void test_vpu3(void);

#endif //VPU_WRAPER_H_
