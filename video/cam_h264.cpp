
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
using namespace std;

#include <unistd.h>

#include "cam_cap.h"
#include "vpucls.h"
#include "toollib.h"
#include "frame_mng.h"
#include "yuvaddtime.h"

#include "cam_h264.h"


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
		cout <<"cam_h264 pthread_join(m_thread,NULL); "<<endl;
	}

	if(mp_cam){
		cam_cap*pcam = (cam_cap*)mp_cam;
		delete pcam; mp_cam = NULL;
		cout<<"delete pcam"<<endl;
	}

	if(mp_vpu){
		vpucls *pvpu = (vpucls*)mp_vpu;
		delete pvpu; mp_vpu = NULL;
		cout<<"delete mp_vpu"<<endl;
	}

	delete (frame_mng*)mp_h264queue;
}

/////return 0 ok   -1 err
int cam_h264::init(void){
	m_thread_exitflag = false;
	mp_cam = NULL;
	mp_vpu = NULL;
	m_thread = 0;
	int iret;

	mp_h264queue = new frame_mng();

	vpucls *pvpu = new vpucls(m_w,m_h,m_f);
	mp_vpu = pvpu;
	iret = pvpu->init();
	if(iret!=0){
		cerr<<__FILE__<<" "<<__FUNCTION__<<" vpu init err"<<endl;
		delete pvpu; mp_vpu = NULL;
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
	vpucls *pvpu = (vpucls*)mp_vpu;
	cam_cap*pcam = (cam_cap*)mp_cam;
	vector<unsigned char>vuc(1920*1080*4);
	time_t last_sec=0;
	int iret;
int idx = 0;
	while(!m_thread_exitflag){
		void*pcamdata;
		iret = pcam->query_frame_p(&pcamdata);
		if(iret!=0){
			cerr<<__FILE__<<" "<<__FUNCTION__<<" pcam->query_frame err"<<endl;
			sleep(1);
			exit(-1);
		}

		time_t sec=syssec();//cout<<sec<<" "<<idx++<<endl;

		bool bIFrame=false;
		if(sec!=last_sec){
			last_sec = sec;
			bIFrame = true;
		}

		struct timeval time_ms;
		gettimeofday(&time_ms, NULL);
		//yuv420addtime((unsigned char *)pcamdata,m_w,m_h,time_ms.tv_sec,time_ms.tv_usec/1000);///picture add time


		shared_ptr<TYPE_VU8> spv = make_shared<TYPE_VU8>();
		iret = pvpu->enc(pcamdata,m_w*m_h*3/2,bIFrame,*spv);
		if(iret!=0){
			cerr<<__FILE__<<" "<<__FUNCTION__<<" pvpu->enc err"<<endl;
			sleep(1);
			exit(-1);
		}
		((frame_mng*)mp_h264queue)->add_frame(sec,spv);
	}
}

////return 0 no data sleep     1 have data
int cam_h264::get_sec(int sec,vector<TYPE_VU8> &v_sec){
	return ((frame_mng*)mp_h264queue)->get_sec_frame(sec,v_sec);
}


int cam_h264::get_sps(std::vector<unsigned char>&sps){
	return ((vpucls*)mp_vpu)->get_sps(sps);
}
int cam_h264::get_pps(std::vector<unsigned char>&pps){
	return ((vpucls*)mp_vpu)->get_pps(pps);
}
void tst_cam264(void){
	int w = 1280,h=720,f=30;
	char *dev = "/dev/video1";

	cam_h264 cam264(dev,w,h,f);
	cam264.init();

	while(1){
	
		char fname[64];
		sprintf(fname,"%d.264",syssec());
		ofstream outh264(fname, ios::out | ios::binary);
		vector<unsigned char> v_sps,v_pps;
		cam264.get_sps(v_sps);cam264.get_pps(v_pps);
		outh264.write((char*)&v_sps[0], v_sps.size()); outh264.write((char*)&v_pps[0], v_pps.size());
		int sec = syssec();
		for(int i = 0; i < 10; ){
			int iret ;
			vector<vector<unsigned char>> vv_sec;
			iret = cam264.get_sec(sec,vv_sec);
			if(iret == 0){
				sleep(1);
				continue;
			}
			i++;
			sec++;
			for(int i = 0; i < vv_sec.size(); i++){
				outh264.write((char*)&vv_sec[i][0],vv_sec[i].size());
			}
		}
		cout <<fname<<endl;
	
		sleep(10);
	}
}

