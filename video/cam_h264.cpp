
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#include <unistd.h>
#include <sys/sysinfo.h>

#include "cam_cap.h"
#include "vpucls.h"

#include "cam_h264.h"

class cam_h264{
public:
	cam_h264(const char *dev, int width, int height, int frame_rate);
	~cam_h264(void);
	int init(void);
	void de_init(void);
	int run(void);

	int m_w;
	int m_h;
	int m_f;
	std::string m_dev;
	void* mp_thread;
	pthread_t m_thread;
	bool m_thread_exitflag;

	void*mp_cam;
	void*mp_h264;
};


cam_h264::cam_h264(const char *dev, int width, int height, int frame_rate){
	m_dev = dev;
	m_w = width;
	m_h = height;
	m_f = frame_rate;
}

cam_h264::~cam_h264(void){
	de_init();
}


static void * runpp(void* para){
	cam_h264 *pcam264 = (cam_h264 *)para;
	pcam264->run();
	pthread_exit(0);
}

void cam_h264::de_init(void){
	m_thread_exitflag = true;

	if(m_thread){
		pthread_join(m_thread,NULL);
		cout <<"pthread_join(m_thread,NULL); "<<endl;
	}

	if(mp_cam){
		cam_cap*pcam = (cam_cap*)mp_cam;
		delete pcam; mp_cam = NULL;
		cout<<"delete pcam"<<endl;
	}

	if(mp_h264){
		vpucls *pvpu = (vpucls*)mp_h264;
		delete pvpu; mp_h264 = NULL;
		cout<<"delete mp_h264"<<endl;
	}
}

/////return 0 ok   -1 err
int cam_h264::init(void){
	m_thread_exitflag = false;
	mp_cam = NULL;
	mp_h264 = NULL;
	m_thread = 0;
	int iret;

	vpucls *pvpu = new vpucls(m_w,m_h,m_f);
	mp_h264 = pvpu;
	iret = pvpu->init();
	if(iret!=0){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" vpu init err"<<endl;
		delete pvpu; mp_h264 = NULL;
		de_init();
		return -1;
	}
	

	cam_cap*pcam = new cam_cap(m_dev.c_str(),m_w,m_h,m_f);
	mp_cam = pcam;
	iret = pcam->init();
	if(iret!=0){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" cam_cap init err"<<endl;
		delete pcam; mp_cam = NULL;
		de_init();
		return -1;
	}

	iret = pthread_create(&m_thread,NULL,runpp,this);
	if(iret!=0){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" pthread_create err"<<endl;
		m_thread = 0;
		de_init();
		return -1;
	}
}



int cam_h264::run(void){
	vpucls *pvpu = (vpucls*)mp_h264;
	cam_cap*pcam = (cam_cap*)mp_cam;
	vector<unsigned char>vuc(1920*1080*4);
	time_t last_sec;
	int iret;

ofstream outh264("campp-.264", ios::out | ios::binary);
{
vector<unsigned char> sps,pps;
pvpu->get_sps(sps);
pvpu->get_pps(pps);
outh264.write((char*)&sps[0], sps.size()); outh264.write((char*)&pps[0], pps.size());
}
	
	//outh264.write((char*)&v_sps[0], v_sps.size()); outh264.write((char*)&v_pps[0], v_pps.size());

	while(!m_thread_exitflag){
		void*pcamdata;
		//iret = pcam->query_frame(&vuc[0]);
iret = pcam->query_frame_p(&pcamdata);
		if(iret!=0){
			cerr<<__FILE__<<" "<<__FUNCTION__<<" pcam->query_frame err"<<endl;
		}

		struct sysinfo s_info;
		sysinfo(&s_info);
		time_t sec=s_info.uptime;

		bool bIFrame=false;
		if(sec!=last_sec){
			last_sec = sec;
			bIFrame = true;
		}

		vector<unsigned char>v_h264;
//memcpy(&vuc[0],pcamdata,m_w*m_h*3/2);
		//iret = pvpu->enc((char*)&vuc[0],m_w*m_h*3/2,bIFrame,v_h264);
iret = pvpu->enc(pcamdata,m_w*m_h*3/2,bIFrame,v_h264);
		if(iret!=0){
			cerr<<__FILE__<<" "<<__FUNCTION__<<" pvpu->enc err"<<endl;
		}
outh264.write((char*)&v_h264[0], v_h264.size());
		cout <<"run "<<endl;//sleep(1);
	}
}

void tst_cam264(void){
	int w = 640,h=480,f=30;
	char *dev = "/dev/video0";

	cam_h264 cam264(dev,w,h,f);
	cam264.init();

while(1){
sleep(1);
}
}

