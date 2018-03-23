

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
			cout <<" deinit cam_h264 "<<endl;
			delete p_cam_h264; p_cam_h264 = NULL;
		}
	}

	de_init_snd();
	pthread_mutex_destroy(&m_mutex);
}

int video_mng::init(void){

	m_thread_exitflag = false;
	memset(m_video_para,0,sizeof(m_video_para));
	memset(&m_snd_para,0,sizeof(m_snd_para));

	pthread_mutex_init(&m_mutex, NULL);
	int iret;

	iret = init_snd();
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" init_snd err "<<endl;
		de_init();
		return -1;
	}

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

int video_mng::init_snd(void){
	memset(&m_snd_para,0,sizeof(m_snd_para));

	int iret;
	snd_pcm *p_pcm = new snd_pcm(22050,1);
	iret = p_pcm->init();
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" p_pcm->init err "<<endl;
		delete p_pcm;
		de_init();
		return -1;
	}
	m_snd_para.mp_pcm = p_pcm;

	m_snd_para.mp_pcmqueue = new frame_mng();

	struct thread_para * p_th_para;
	p_th_para = new struct thread_para;
	p_th_para->obj = this;
	p_th_para->ptf = &video_mng::readpcm;
	iret = pthread_create(&m_snd_para.m_thread_pcm,NULL,c_thread,p_th_para);
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" m_snd_para.m_thread err "<<endl;
		m_snd_para.m_thread_pcm = 0;
		delete p_th_para;
		de_init();
		return -1;
	}

	//struct thread_para * p_th_para;
	p_th_para = new struct thread_para;
	p_th_para->obj = this;
	p_th_para->ptf = &video_mng::aacenc;
	iret = pthread_create(&m_snd_para.m_thread_aac,NULL,c_thread,p_th_para);
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" m_snd_para.m_thread_aac err "<<endl;
		m_snd_para.m_thread_aac = 0;
		delete p_th_para;
		de_init();
		return -1;
	}

	return 0;
}

void video_mng::de_init_snd(void){

	if(m_snd_para.m_thread_aac){
		pthread_join(m_snd_para.m_thread_aac,NULL);
		cout <<" m_snd_para.m_thread_aac pthread_join "<<endl;
		m_snd_para.m_thread_aac = 0;
	}

	if(m_snd_para.m_thread_pcm){
		pthread_join(m_snd_para.m_thread_pcm,NULL);
		cout <<" m_snd_para pthread_join "<<endl;
		m_snd_para.m_thread_pcm = 0;
	}

	frame_mng *p_pcmqueue = (frame_mng *)m_snd_para.mp_pcmqueue;
	if(p_pcmqueue){
		cout <<" de_init pcm que"<<endl;
		delete p_pcmqueue; p_pcmqueue = NULL;
	}
	
	snd_pcm *p_pcm = (snd_pcm *)(m_snd_para.mp_pcm);
	if(p_pcm){
		cout <<" de_init pcm "<<endl;
		delete p_pcm; p_pcm = NULL;
	}
}



void video_mng::recmp4(int idx){
	cam_h264 * p_cam264 = (cam_h264 *)(m_video_para[idx].mp_cam264);

	mymp4 *pmp4=NULL;
	char fname[64];
	bool bret;
	int iret;
	while(!m_thread_exitflag){

		//if(m_video_para[idx].m_rec_flag==false){
		//	usleep(10*1000);
		//	continue;
		//}

		//m_video_para[idx].m_rec_flag = false;

//sleep(1);
//continue;

switch(m_video_para[idx].m_vRECSTATE)
{
case E_RECSTATE_IDLE:
	usleep(20*1000);
	break;

case E_RECSTATE_CMD:
	cout<<" rec state cmd "<<endl;
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
//cout <<"iret "<<iret<<endl;
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
	cout<<" E_RECSTATE_MP4_ADD_AAC "<<endl;
	{
	int input_samples;
	std::vector<char>v_decoder_info;
	std::deque<vector<char>>dqv_aac;
	iret = getaac(m_video_para[idx].m_rec_begin_syssec,input_samples,v_decoder_info,dqv_aac);
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
	cout<<" E_RECSTATE_DEINIT "<<endl;
	if(pmp4){
		delete pmp4;pmp4 = NULL;
	}
	pthread_mutex_lock(&m_mutex);
m_video_para[idx].m_vRECSTATE = E_RECSTATE_END;
	pthread_mutex_unlock(&m_mutex);
	break;

case E_RECSTATE_END:
	cout<<" E_RECSTATE_END "<<endl;
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

		char fname[64];
		sprintf(fname,"%d-%d.mp4",(int)m_video_para[idx].m_rec_begin_syssec,idx);
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
		vector<char> v_sps2,v_pps2;
		v_sps2.insert(v_sps2.end(),v_sps.begin()+4,v_sps.end());
		v_pps2.insert(v_pps2.end(),v_pps.begin()+4,v_pps.end());
		bret=o_mp4.init_v(p_cam264->m_w,p_cam264->m_h,p_cam264->m_f,v_sps2,v_pps2);
		if(!bret){
			cout<<"o_mp4.init_v false "<<fname<<endl;
			sleep(1);exit(-1);
		}

		int sec = m_video_para[idx].m_rec_begin_syssec;
		for(int i = 0; i < V_REC_SEC; ){
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

{
#if 0
		myaac o_aac(SND_HZ,SND_CHANNEL,SND_BITS);
		bret=o_aac.init();
		if(!bret){
			cout<<"o_aac.init false "<<fname<<endl;
			sleep(1);exit(-1);
		}
		bret=o_mp4.init_a(SND_HZ,o_aac.get_input_samples(),o_aac.get_decoder_info());
		if(!bret){
			cout<<"o_mp4.init_a false "<<fname<<endl;
			sleep(1);exit(-1);
		}


		frame_mng*p_pcmque = (frame_mng*)m_snd_para.mp_pcmqueue;
		for(int i = 0; i < V_REC_SEC; ){
			int iret ;
			vector<vector<unsigned char>> vv_sec;
			iret = p_pcmque->get_sec_frame(sec,vv_sec);
			if(iret == 0){
				sleep(1);
				continue;
			}
			i++;
			sec++;

			for(int i = 0; i < vv_sec.size(); i++){
				vector<char> v_aac;
				o_aac.pcm2aac((char*)&vv_sec[i][0],vv_sec[i].size()/2,v_aac);
				if (v_aac.size()){
					bret = o_mp4.write_a(&v_aac[0+7],v_aac.size()-7);
					if(!bret){
						cout<<"o_mp4.write_v false "<<fname<<endl;
						sleep(1);exit(-1);
					}
				}
			}
		}
#endif
}
	}

	if(pmp4){
		delete pmp4;pmp4 = NULL;
	}
}

void video_mng::readpcm(int iarg){
	snd_pcm *p_pcm = (snd_pcm *)(m_snd_para.mp_pcm);
	frame_mng *p_pcmque=(frame_mng *)m_snd_para.mp_pcmqueue;

	cout <<"p_pcm->size "<<p_pcm->size<<endl;
	int idx = 0;
	int iret;
	while(!m_thread_exitflag){
		iret = p_pcm->read();
		if(iret != 0){
			cout<<" p_pcm->read false "<<endl;
			sleep(1);exit(-1);
		}

		time_t sec=syssec();
		shared_ptr<TYPE_VU8> spv = make_shared<TYPE_VU8>();
		spv->insert(spv->end(),p_pcm->buffer,p_pcm->buffer+p_pcm->size);
		p_pcmque->add_frame(sec,spv);
	}
}

void video_mng::aacenc(int iarg){

	cout <<"aacenc... "<<endl;

	myaac *paac;
	frame_mng *p_pcmque;
	bool bret;
	while(!m_thread_exitflag){
switch(m_snd_para.m_sndRECSTATE)
{
case E_RECSTATE_IDLE:
	usleep(20*1000);
	break;
case E_RECSTATE_CMD:
	cout<<" state cmd "<<endl;
	pthread_mutex_lock(&m_mutex);
	m_snd_para.m_sndRECSTATE = E_RECSTATE_INIT;
	pthread_mutex_unlock(&m_mutex);
	break;
case E_RECSTATE_INIT:
	pthread_mutex_lock(&m_mutex);
	cout<<" E_RECSTATE_INIT "<<endl;
	m_snd_para.mp_aac = NULL;
	paac = new myaac(SND_HZ,SND_CHANNEL,SND_BITS);
	bret=paac->init();
	if(!bret){
		cout<<__FILE__<<" "<<__FUNCTION__<<" paac->init false "<<endl;
		sleep(1);exit(-1);
	}
	m_snd_para.mp_aac=paac;

	m_snd_para.mp_aacque=new deque<vector<char>>;
	m_snd_para.m_hadencsec = 0;

	if(m_video_para[0].m_vRECSTATE == E_RECSTATE_IDLE || m_video_para[0].m_vRECSTATE == E_RECSTATE_END){
		m_video_para[0].m_vRECSTATE = E_RECSTATE_CMD;
		m_video_para[0].m_rec_begin_syssec = m_snd_para.m_rec_begin_syssec;
	}
	if(m_video_para[1].m_vRECSTATE == E_RECSTATE_IDLE || m_video_para[1].m_vRECSTATE == E_RECSTATE_END){
		m_video_para[1].m_vRECSTATE = E_RECSTATE_CMD;
		m_video_para[1].m_rec_begin_syssec = m_snd_para.m_rec_begin_syssec;
	}
	m_snd_para.m_sndRECSTATE = E_RECSTATE_RUN;
	pthread_mutex_unlock(&m_mutex);

	break;

case E_RECSTATE_RUN:

	paac =(myaac*) m_snd_para.mp_aac;
	p_pcmque=(frame_mng*)m_snd_para.mp_pcmqueue;
	if(m_snd_para.m_hadencsec<V_REC_SEC){
		int iret;
		vector<vector<unsigned char>> vv_sec;
		iret = p_pcmque->get_sec_frame(m_snd_para.m_rec_begin_syssec+m_snd_para.m_hadencsec,vv_sec);
		//cout<<" getok iret "<<iret<<endl;
		if(iret == 0){
			usleep(20*1000);
			break;
		}
		m_snd_para.m_hadencsec++;

		//cout<<" m_snd_para.m_hadencsec "<<m_snd_para.m_hadencsec<<endl;
		//cout <<"vv_sec.size() "<<vv_sec.size()<<endl;
		for(int i = 0; i < vv_sec.size(); i++){
			vector<char> v_aac;
			paac->pcm2aac((char*)&vv_sec[i][0],vv_sec[i].size()/2,v_aac);
			if (v_aac.size()){
				pthread_mutex_lock(&m_mutex);
				m_snd_para.mp_aacque->push_back(v_aac);
				pthread_mutex_unlock(&m_mutex);
			}
			usleep(2*1000);
		}
	}else{
		pthread_mutex_lock(&m_mutex);
		m_snd_para.m_sndRECSTATE = E_RECSTATE_AACOK;
//cout << "m_snd_para.m_rec_begin_syssec "<<m_snd_para.m_rec_begin_syssec<<" syssec() " << syssec()<<endl;

		pthread_mutex_unlock(&m_mutex);
	}
	break;
case E_RECSTATE_AACOK:
	//cout<<" E_RECSTATE_AACOK "<<endl;
	if(syssec()>m_snd_para.m_rec_begin_syssec + V_REC_SEC + AACOK_WAIT_SEC){
		pthread_mutex_lock(&m_mutex);
		m_snd_para.m_sndRECSTATE = E_RECSTATE_DEINIT;
		m_snd_para.m_rec_begin_syssec = 0;
		pthread_mutex_unlock(&m_mutex);	
//cout << " syssec() " << syssec()<<endl;
	}else{
		sleep(1);
	}
	break;
case E_RECSTATE_DEINIT:
	//cout<<" E_RECSTATE_DEINIT "<<endl;
	pthread_mutex_lock(&m_mutex);

	if(m_snd_para.mp_aacque){
		delete m_snd_para.mp_aacque; m_snd_para.mp_aacque = NULL;
	}

	paac =(myaac*) m_snd_para.mp_aac;
	if(paac){
		delete paac;paac = NULL;
	}
	m_snd_para.m_sndRECSTATE = E_RECSTATE_END;

	pthread_mutex_unlock(&m_mutex);
	break;

case E_RECSTATE_END:
	cout<<" E_RECSTATE_END "<<endl;
	pthread_mutex_lock(&m_mutex);
	m_snd_para.m_sndRECSTATE = E_RECSTATE_IDLE;
	pthread_mutex_unlock(&m_mutex);
	break;

default:
	cout<<__FILE__<<" "<<__FUNCTION__<<" can't here "<<endl;
	sleep(1);exit(-1);
	break;
}

	usleep(5*1000);

	}

	if(m_snd_para.mp_aacque){
		delete m_snd_para.mp_aacque; m_snd_para.mp_aacque = NULL;
	}

	paac =(myaac*) m_snd_para.mp_aac;
	if(paac){
		delete paac;paac = NULL;
	}

}

////-1 NO AAC    0 WAIT  1 0K
int video_mng::getaac(int sec,int &input_samples,std::vector<char>&v_decoder_info,std::deque<vector<char>>&dqv_aac){
	int iret ;
	pthread_mutex_lock(&m_mutex);
	if(m_snd_para.m_rec_begin_syssec != sec){
		iret = -1;
	}
	if(m_snd_para.m_sndRECSTATE != E_RECSTATE_AACOK){
		iret = 0;
	}else{
		myaac*paac =(myaac*) m_snd_para.mp_aac;
		input_samples = paac->get_input_samples();
		v_decoder_info=paac->get_decoder_info();
		dqv_aac = *m_snd_para.mp_aacque;
		iret = 1;
	}
	pthread_mutex_unlock(&m_mutex);

	return iret;
}


void video_mng::rec(time_t sec){
	cout<<"video_mng::rec "<<sec<<endl;
	pthread_mutex_lock(&m_mutex);
	if(m_snd_para.m_sndRECSTATE == E_RECSTATE_IDLE || m_snd_para.m_sndRECSTATE == E_RECSTATE_END){
		m_snd_para.m_sndRECSTATE = E_RECSTATE_CMD;
		m_snd_para.m_rec_begin_syssec = sec;
	}
	pthread_mutex_unlock(&m_mutex);
}

void tst_vmng(void){
	video_mng vmng;
	vmng.init();
	while(1){
		sleep(20);
//sleep(2);
		vmng.rec(syssec()-V_BACK_SEC);
		//sleep(180);
	}
}

