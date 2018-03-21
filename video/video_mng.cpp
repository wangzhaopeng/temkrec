

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include <unistd.h>

#include "cam_h264.h"

#include "toollib.h"
#include "mymp4.h"
#include "video_mng.h"


typedef void (video_mng::*pthread_f)(int);
struct thread_para{
	video_mng * obj;
	pthread_f ptf;
	int idx ;
};
static void * c_thread(void* para)
{
	struct thread_para th_para;
	struct thread_para *pth_para = (struct thread_para *)para;
	th_para = *pth_para;
	delete (struct thread_para *)para;

	video_mng *pvmng;
	pvmng = th_para.obj;
	pthread_f ptf = th_para.ptf;
	(pvmng->*ptf)(th_para.idx);

	pthread_exit(0);
	return NULL;
}

video_mng::~video_mng(void){
	de_init();
}
int video_mng::de_init(void){
	m_thread_exitflag = true;

	for(int i = 0; i < 2; i++){
		if(m_video_para[i].m_thread){
			pthread_join(m_video_para[i].m_thread,NULL);
			cout <<"video_mng pthread_join"<<endl;
			m_video_para[i].m_thread = 0;
		}
		
		cam_h264 *p_cam_h264 = (cam_h264 *)(m_video_para[i].mp_cam264);
		if(p_cam_h264){
			delete p_cam_h264; p_cam_h264 = NULL;
		}
	}
}

int video_mng::init(void){
	m_thread_exitflag = false;

	memset(m_video_para,0,sizeof(m_video_para));

	int iret;
	cam_h264 *p_cam264_0,*p_cam264_1;

	p_cam264_0 = new cam_h264("/dev/video0",640,480,30);
	iret =p_cam264_0->init();
	if(iret!=0){
		delete p_cam264_0;
	}
	m_video_para[0].mp_cam264 = p_cam264_0;

	p_cam264_1 = new cam_h264("/dev/video1",1280,720,30);
	iret =p_cam264_1->init();
	if(iret!=0){
		delete p_cam264_1;
	}
	m_video_para[1].mp_cam264 = p_cam264_1;

	for(int i = 0; i<2;i++){
		struct thread_para * p_th_para;
		p_th_para = new struct thread_para;
		p_th_para->obj = this;
		p_th_para->ptf = &video_mng::recmp4;
		p_th_para->idx=i;
		iret = pthread_create(&m_video_para[i].m_thread,NULL,c_thread,p_th_para);
		if(iret!=0){
			printf("create thread err\n");
			m_video_para[i].m_thread = 0;
			delete p_th_para;
			de_init();
			return -1;
		}
	}
}


void video_mng::recmp4(int idx){
	cam_h264 * p_cam264 = (cam_h264 *)(m_video_para[idx].mp_cam264);
	while(!m_thread_exitflag){

		if(m_video_para[idx].m_rec_flag){
			m_video_para[idx].m_rec_flag = false;
			cout <<idx<<" "<<m_video_para[idx].m_rec_sec<<endl;

			char fname[64];
			sprintf(fname,"%d-%d.mp4",(int)m_video_para[idx].m_rec_sec,idx);
			cout<<fname<<endl;

			mymp4 o_mp4;
			bool bret=o_mp4.init(fname);
			if(!bret){
				cout<<"o_mp4.init false "<<fname<<endl;
				sleep(1);exit(-1);
			}
			int iret;
			vector<unsigned char> v_sps,v_pps;
			iret = p_cam264->get_sps(v_sps);
			if(iret!=0){
				cout<<"p_cam264->get_sps(v_sps); err   cam idx:"<<idx<<endl;
				sleep(1);exit(-1);
			}
			iret = p_cam264->get_pps(v_pps);
			if(iret!=0){
				cout<<"p_cam264->get_pps(v_pps); err   cam idx:"<<idx<<endl;
				sleep(1);exit(-1);
			}
			vector<unsigned char> v_sps2,v_pps2;
			v_sps2.insert(v_sps2.end(),v_sps.begin()+4,v_sps.end());
			v_pps2.insert(v_pps2.end(),v_pps.begin()+4,v_pps.end());
			o_mp4.init_v(p_cam264->m_w,p_cam264->m_h,p_cam264->m_f,v_sps2,v_pps2);

			int sec = syssec();
			for(int i = 0; i < 10; ){
				int iret ;
				vector<vector<unsigned char>> vv_sec;
				iret = p_cam264->get_sec(sec,vv_sec);
				if(iret == 0){
					sleep(1);
					continue;
				}
				i++;
				sec++;
				for(int i = 0; i < vv_sec.size(); i++){
					bret = o_mp4.write_v((char*)&vv_sec[i][4], vv_sec[i].size()-4);
					if(!bret){
							cout<<"o_mp4.write_v false "<<fname<<endl;
							sleep(1);exit(-1);
					}
				}
			}
		}
		usleep(100*1000);
	}
}


void video_mng::rec(time_t sec){
	cout<<"video_mng::rec "<<sec<<endl;

	m_video_para[0].m_rec_flag=m_video_para[1].m_rec_flag=true;
	m_video_para[0].m_rec_sec=m_video_para[1].m_rec_sec=sec;
}

void tst_vmng(void){
	video_mng vmng;
	vmng.init();
	while(1){
		sleep(20);
		vmng.rec(time(NULL));
	}
}

