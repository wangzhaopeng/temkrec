

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include <unistd.h>

#include "cam_h264.h"

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
	//th_para.obj = pth_para->obj;
	//th_para.ptf = pth_para->ptf;
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

	if(m_thread_recmp4[0]){
		pthread_join(m_thread_recmp4[0],NULL);
		cout <<"video_mng pthread_join"<<endl;
	}

	if(m_thread_recmp4[1]){
		pthread_join(m_thread_recmp4[1],NULL);
		cout <<"video_mng pthread_join"<<endl;
	}

	if(mp_cam264_0){
		delete (cam_h264 *)mp_cam264_0; mp_cam264_0 = NULL;
	}
	if(mp_cam264_1){
		delete (cam_h264 *)mp_cam264_1; mp_cam264_1 = NULL;
	}
}

int video_mng::init(void){
	mp_cam264_0 = NULL;
	mp_cam264_1 = NULL;
	m_thread_exitflag = false;

	m_recmp4_para[0].m_rec_flag=m_recmp4_para[1].m_rec_flag=false;
	m_thread_recmp4[0] = 0;m_thread_recmp4[1]=0;
	int iret;
	cam_h264 *p_cam264_0,*p_cam264_1;

	p_cam264_0 = new cam_h264("/dev/video0",640,480,30);
	iret =p_cam264_0->init();
	if(iret!=0){
		delete p_cam264_0;
	}
	mp_cam264_0 = p_cam264_0;

	p_cam264_1 = new cam_h264("/dev/video1",1280,720,30);
	iret =p_cam264_1->init();
	if(iret!=0){
		delete p_cam264_1;
	}
	mp_cam264_1 = p_cam264_1;

{
	struct thread_para * p_th_para;
	p_th_para = new struct thread_para;
	p_th_para->obj = this;
	p_th_para->ptf = &video_mng::recmp4;
	p_th_para->idx=0;
	iret = pthread_create(&m_thread_recmp4[0],NULL,c_thread,p_th_para);
	if(iret!=0){
		printf("create thread err\n");
		m_thread_recmp4[0] = 0;
		delete p_th_para;
		de_init();
		return -1;
	}
}

{
	struct thread_para * p_th_para;
	p_th_para = new struct thread_para;
	p_th_para->obj = this;
	p_th_para->ptf = &video_mng::recmp4;
	p_th_para->idx=1;
	iret = pthread_create(&m_thread_recmp4[1],NULL,c_thread,p_th_para);
	if(iret!=0){
		printf("create thread err\n");
		m_thread_recmp4[1] = 0;
		delete p_th_para;
		de_init();
		return -1;
	}
}
}


void video_mng::recmp4(int idx){
	while(!m_thread_exitflag){
		if(m_recmp4_para[idx].m_rec_flag){
			m_recmp4_para[idx].m_rec_flag = false;
			cout <<idx<<" "<<m_recmp4_para[idx].m_rec_sec<<endl;
		}
		usleep(1000);
	}
}


void video_mng::rec(time_t sec){
	cout<<"video_mng::rec "<<sec<<endl;

	m_recmp4_para[0].m_rec_flag=m_recmp4_para[1].m_rec_flag=true;
	m_recmp4_para[0].m_rec_sec=m_recmp4_para[1].m_rec_sec=sec;
}

void tst_vmng(void){
	video_mng vmng;
	vmng.init();
	while(1){
		sleep(5);
		vmng.rec(time(NULL));
	}
}

