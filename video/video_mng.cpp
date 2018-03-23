

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
#include "snd_pcm.h"
#include "myaac.h"
#include "frame_mng.h"
#include "pcm_aac.h"
#include "video_mng.h"


#define SND_HZ			22050
#define SND_CHANNEL		1
#define SND_BITS		16

#define V_REC_SEC		10
#define V_BACK_SEC		5

#define AACOK_WAIT_SEC		5

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

	video_mng *pobj;
	pobj = th_para.obj;
	pthread_f ptf = th_para.ptf;
	(pobj->*ptf)(th_para.idx);

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
			cout <<" deinit cam_h264 "<<endl;
			delete p_cam_h264; p_cam_h264 = NULL;
		}
	}


	if(mp_pcm_aac){
		delete (pcm_aac *)mp_pcm_aac;mp_pcm_aac=NULL;
	}

	pthread_mutex_destroy(&m_mutex);
}

int video_mng::init(void){

	m_thread_exitflag = false;
	memset(m_video_para,0,sizeof(m_video_para));
	mp_pcm_aac = NULL;
	pthread_mutex_init(&m_mutex, NULL);

	int iret;
	pcm_aac *p_pcm_aac = new pcm_aac(SND_HZ,SND_CHANNEL,SND_BITS,V_REC_SEC);
	iret = p_pcm_aac->init();
	if(iret != 0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" p_pcm_aac->init err "<<endl;
		delete p_pcm_aac;
		de_init();
		return -1;
	}
	mp_pcm_aac = p_pcm_aac;

	cam_h264 *p_cam264_0,*p_cam264_1;

	p_cam264_0 = new cam_h264("/dev/video0",640,480,30);
	iret =p_cam264_0->init();
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" p_cam264_0->init err "<<endl;
		delete p_cam264_0;
		de_init();
		return -1;
	}
	m_video_para[0].mp_cam264 = p_cam264_0;

	p_cam264_1 = new cam_h264("/dev/video1",1280,720,30);
	iret =p_cam264_1->init();
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" p_cam264_1->init err "<<endl;
		delete p_cam264_1;
		de_init();
		return -1;
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
			cout <<__FILE__<<" "<<__FUNCTION__<<" create m_video_para[i].m_thread err "<<endl;
			m_video_para[i].m_thread = 0;
			delete p_th_para;
			de_init();
			return -1;
		}
	}
}


void video_mng::recmp4(int idx){
	cam_h264 * p_cam264 = (cam_h264 *)(m_video_para[idx].mp_cam264);

	mymp4 *pmp4=NULL;
	char fname[64];
	bool bret;
	int iret;
	while(!m_thread_exitflag){
		switch(m_video_para[idx].m_vRECSTATE)
		{
		case E_RECSTATE_IDLE:
			usleep(20*1000);
			break;

		case E_RECSTATE_CMD:
			//cout<<" rec state cmd "<<endl;
			pthread_mutex_lock(&m_mutex);
			m_video_para[idx].m_vRECSTATE = E_RECSTATE_INIT;
			pthread_mutex_unlock(&m_mutex);
			break;
		case E_RECSTATE_INIT:
		{	
			sprintf(fname,"%d-%d.mp4",(int)m_video_para[idx].m_rec_begin_syssec,idx);
			cout<<fname<<endl;

			pmp4 = new mymp4;
			bool bret=pmp4->init(fname);
			if(!bret){
				cout<<" o_mp4.init false "<<fname<<endl;
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
			vector<char> v_sps2,v_pps2;
			v_sps2.insert(v_sps2.end(),v_sps.begin()+4,v_sps.end());
			v_pps2.insert(v_pps2.end(),v_pps.begin()+4,v_pps.end());
			bret=pmp4->init_v(p_cam264->m_w,p_cam264->m_h,p_cam264->m_f,v_sps2,v_pps2);
			if(!bret){
				cout<<"o_mp4.init_v false "<<fname<<endl;
				sleep(1);exit(-1);
			}
			m_video_para[idx].m_hadrec264sec = 0;
		}
			pthread_mutex_lock(&m_mutex);
			m_video_para[idx].m_vRECSTATE = E_RECSTATE_RUN;
			pthread_mutex_unlock(&m_mutex);
			break;
		case E_RECSTATE_RUN:
		//cout<<"run "<<endl;
			if(m_video_para[idx].m_hadrec264sec<V_REC_SEC){
				int iret ;
				vector<vector<unsigned char>> vv_sec;
				iret = p_cam264->get_sec(m_video_para[idx].m_rec_begin_syssec+m_video_para[idx].m_hadrec264sec,vv_sec);
				if(iret == 0){
					sleep(1);
					break;
				}
				m_video_para[idx].m_hadrec264sec++;
				for(int i = 0; i < vv_sec.size(); i++){
					bool bret = pmp4->write_v((char*)&vv_sec[i][4], vv_sec[i].size()-4);
					if(!bret){
						cout<<"o_mp4.write_v false "<<fname<<endl;
						sleep(1);exit(-1);
					}
				}
			}else{
				pthread_mutex_lock(&m_mutex);
				m_video_para[idx].m_vRECSTATE = E_RECSTATE_MP4_ADD_AAC;
				pthread_mutex_unlock(&m_mutex);
			}
			break;
		case E_RECSTATE_MP4_ADD_AAC:
			//cout<<" E_RECSTATE_MP4_ADD_AAC "<<endl;
		{
			int input_samples;
			std::vector<char>v_decoder_info;
			std::deque<vector<char>>dqv_aac;
			pcm_aac *p_pcm_aac = (pcm_aac *)mp_pcm_aac;
			iret = p_pcm_aac->getaac(m_video_para[idx].m_rec_begin_syssec,input_samples,v_decoder_info,dqv_aac);
		//cout << " getaac iret "<<iret<<endl;
			if(iret == 0){
				usleep(100*1000);//cout <<" getaac sleep "<<endl;
				break;
			}else if(iret < 0){
				pthread_mutex_lock(&m_mutex);
				m_video_para[idx].m_vRECSTATE = E_RECSTATE_DEINIT;
				pthread_mutex_unlock(&m_mutex);
			}else{
				bret = pmp4->init_a(SND_HZ,input_samples,v_decoder_info);
				if(!bret){
					cout<<"pmp4->init_a false "<<fname<<endl;
					sleep(1);exit(-1);
				}

				for(int i = 0; i < dqv_aac.size(); i++){
					bret = pmp4->write_a(&dqv_aac[i][0+7],dqv_aac[i].size()-7);
					if(!bret){
						cout<<"pmp4->write_a false "<<fname<<endl;
						sleep(1);exit(-1);
					}
					usleep(1*1000);
				}
				pthread_mutex_lock(&m_mutex);
				m_video_para[idx].m_vRECSTATE = E_RECSTATE_DEINIT;
				pthread_mutex_unlock(&m_mutex);
			}

		}
			break;

		case E_RECSTATE_DEINIT:
			//cout<<" E_RECSTATE_DEINIT "<<endl;
			if(pmp4){
				delete pmp4;pmp4 = NULL;
			}
			pthread_mutex_lock(&m_mutex);
			m_video_para[idx].m_vRECSTATE = E_RECSTATE_END;
			pthread_mutex_unlock(&m_mutex);
			break;

		case E_RECSTATE_END:
			//cout<<" E_RECSTATE_END "<<endl;
			pthread_mutex_lock(&m_mutex);
			m_video_para[idx].m_vRECSTATE = E_RECSTATE_IDLE;
			pthread_mutex_unlock(&m_mutex);
			break;

		default:
			cout<<__FILE__<<" "<<__FUNCTION__<<" can't here "<<endl;
			sleep(1);exit(-1);
			break;
		}
		usleep(5*1000);
		continue;
	}

	if(pmp4){
		delete pmp4;pmp4 = NULL;
	}
}


////return 0  busy    1 can rec
int video_mng::rec(time_t sec){
	
	pcm_aac *p_pcm_aac = (pcm_aac *)mp_pcm_aac;
	int iret = p_pcm_aac->enc_cmd(sec);
	if(iret == 0){
		return 0;
	}

	cout<<"video_mng::rec "<<sec<<endl;

	pthread_mutex_lock(&m_mutex);
	if(m_video_para[0].m_vRECSTATE == E_RECSTATE_IDLE || m_video_para[0].m_vRECSTATE == E_RECSTATE_END){
		m_video_para[0].m_vRECSTATE = E_RECSTATE_CMD;
		m_video_para[0].m_rec_begin_syssec = sec;
	}
	if(m_video_para[1].m_vRECSTATE == E_RECSTATE_IDLE || m_video_para[1].m_vRECSTATE == E_RECSTATE_END){
		m_video_para[1].m_vRECSTATE = E_RECSTATE_CMD;
		m_video_para[1].m_rec_begin_syssec = sec;
	}
	pthread_mutex_unlock(&m_mutex);

	return 1;
}

void tst_vmng(void){
	video_mng vmng;
	vmng.init();
	while(1){
		//sleep(20);
sleep(2);
	int iret = vmng.rec(syssec()-V_BACK_SEC);
		//cout<<iret <<endl;
		//sleep(180);
	}
}

