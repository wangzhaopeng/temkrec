#ifndef VPU_WRAPER_H_
#define VPU_WRAPER_H_


class VPUWraper
{
protected:
	
public:
	VPUWraper()  {}
	~VPUWraper() {}

public:
	static int Init();
	static int Fini();

	
	int OpenEncoder(int nYUV420Width,int nYUV420Height,int frameRate=30);


	int EncoderGetSPS(char* szSPS,int &nlen);
	int EncoderGetPPS(char* szPPS,int &nlen);


	int EncoderOneFrame(const char* szYUV420Data,int nYUV420Datalen,bool bIFrame,char* szH264Data,int &nH264Datalen);

	
	int CloseEncoder();

private:
	long m_lHandle;
};

void test_vpu(void);

#endif //VPU_WRAPER_H_
