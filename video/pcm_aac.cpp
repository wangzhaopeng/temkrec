
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

#include <unistd.h>

#include "toollib.h"
#include "mymp4.h"
#include "snd_pcm.h"
#include "myaac.h"
#include "frame_mng.h"
#include "pcm_aac.h"


#define AACOK_WAIT_SEC		5


typedef void (pcm_aac::*pthread_f)(int);
struct thread_para{
	pcm_aac * obj;
	pthread_f ptf;
	int idx ;
};
static void * c_thread(void* para)
{
	struct thread_para th_para;
	struct thread_para *pth_para = (struct thread_para *)para;
	th_para = *pth_para;
	delete (struct thread_para *)para;

	pcm_aac *pobj;
	pobj = th_para.obj;
	pthread_f ptf = th_para.ptf;
	(pobj->*ptf)(th_para.idx);

	pthread_exit(0);
	return NULL;
}


pcm_aac::pcm_aac(int hz, int channel, int bits,int tsec){
	m_channel = channel;
	m_hz = hz;
	m_bits = bits;
	m_total_sec = tsec;
}

pcm_aac::~pcm_aac(void){
	de_init();
}

void pcm_aac::de_init(void){
	m_thread_exitflag = true;
	if(m_thread_aac){
		pthread_join(m_thread_aac,NULL);
		cout <<" m_snd_para.m_thread_aac pthread_join "<<endl;
		m_thread_aac = 0;
	}

	if(m_thread_pcm){
		pthread_join(m_thread_pcm,NULL);
		cout <<" m_snd_para pthread_join "<<endl;
		m_thread_pcm = 0;
	}

	if(mp_pcmqueue){
		cout <<" de_init pcm que"<<endl;
		delete (frame_mng *)mp_pcmqueue; mp_pcmqueue = NULL;
	}

	if(mp_pcm){
		cout <<" de_init pcm "<<endl;
		delete (snd_pcm *)mp_pcm; mp_pcm = NULL;
	}

	pthread_mutex_destroy(&m_mutex);
}
int pcm_aac::init(void){
	mp_pcm = NULL;
	mp_pcmqueue =NULL;
	m_thread_pcm = 0;
	m_thread_aac = 0;
	m_thread_exitflag = false;
	m_aacencstate = 0;
	pthread_mutex_init(&m_mutex, NULL);

	int iret;
	snd_pcm *p_pcm = new snd_pcm(22050,1);
	iret = p_pcm->init();
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" p_pcm->init err "<<endl;
		delete p_pcm;
		de_init();
		return -1;
	}
	mp_pcm = p_pcm;

	mp_pcmqueue = new frame_mng();

	struct thread_para * p_th_para;
	p_th_para = new struct thread_para;
	p_th_para->obj = this;
	p_th_para->ptf = &pcm_aac::readpcm;
	iret = pthread_create(&m_thread_pcm,NULL,c_thread,p_th_para);
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" m_thread_pcm err "<<endl;
		m_thread_pcm = 0;
		delete p_th_para;
		de_init();
		return -1;
	}

	//struct thread_para * p_th_para;
	p_th_para = new struct thread_para;
	p_th_para->obj = this;
	p_th_para->ptf = &pcm_aac::aacenc;
	iret = pthread_create(&m_thread_aac,NULL,c_thread,p_th_para);
	if(iret!=0){
		cout <<__FILE__<<" "<<__FUNCTION__<<" m_snd_para.m_thread_aac err "<<endl;
		m_thread_aac = 0;
		delete p_th_para;
		de_init();
		return -1;
	}
}

void pcm_aac::readpcm(int){
	snd_pcm *p_pcm = (snd_pcm *)mp_pcm;
	frame_mng *p_pcmque=(frame_mng *)mp_pcmqueue;

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



enum E_AACENCSTATE {
	E_AACENCSTATE_IDLE=0,
	E_AACENCSTATE_CMD,
	E_AACENCSTATE_INIT,
	E_AACENCSTATE_RUN,
	E_AACENCSTATE_AACOK,
	E_AACENCSTATE_DEINIT,
	E_AACENCSTATE_END
};

void pcm_aac::aacenc(int){
	myaac *paac=NULL;
	frame_mng *p_pcmque;
	bool bret;
	while(!m_thread_exitflag){
		switch(m_aacencstate)
		{
		case E_AACENCSTATE_IDLE:
			usleep(20*1000);
			break;
		case E_AACENCSTATE_CMD:
			pthread_mutex_lock(&m_mutex);
			m_aacencstate = E_AACENCSTATE_INIT;
			pthread_mutex_unlock(&m_mutex);
			break;
		case E_AACENCSTATE_INIT:
			//cout<<" E_AACENCSTATE_INIT "<<endl;
			paac = new myaac(m_hz,m_channel,m_bits);
			bret=paac->init();
			if(!bret){
				cout<<__FILE__<<" "<<__FUNCTION__<<" paac->init false "<<endl;
				sleep(1);exit(-1);
			}

			m_aacinput_samples = paac->get_input_samples();
			mv_decoder_info = paac->get_decoder_info();

			m_aacque.clear();
			m_hadencsec = 0;
			m_read_times = 0;

			pthread_mutex_lock(&m_mutex);
			m_aacencstate = E_AACENCSTATE_RUN;
			pthread_mutex_unlock(&m_mutex);

			break;

		case E_AACENCSTATE_RUN:
			p_pcmque=(frame_mng*)mp_pcmqueue;
			if(m_hadencsec<m_total_sec){
				int iret;
				vector<vector<unsigned char>> vv_sec;
				iret = p_pcmque->get_sec_frame(m_rec_begin_syssec+m_hadencsec,vv_sec);
				if(iret == 0){
					usleep(20*1000);
					break;
				}
				m_hadencsec++;

				for(int i = 0; i < vv_sec.size(); i++){
					vector<char> v_aac;
					paac->pcm2aac((char*)&vv_sec[i][0],vv_sec[i].size()/2,v_aac);
					if (v_aac.size()){
						pthread_mutex_lock(&m_mutex);
						m_aacque.push_back(v_aac);
						pthread_mutex_unlock(&m_mutex);
					}
					usleep(2*1000);
				}
			}else{
				pthread_mutex_lock(&m_mutex);
				m_aacencstate = E_AACENCSTATE_AACOK;
				pthread_mutex_unlock(&m_mutex);
			}
			break;
		case E_AACENCSTATE_AACOK:
			//cout<<" E_AACENCSTATE_AACOK "<<endl;
			if(syssec()>m_rec_begin_syssec + m_total_sec + AACOK_WAIT_SEC || m_read_times>=2){
				pthread_mutex_lock(&m_mutex);
				m_aacencstate = E_AACENCSTATE_DEINIT;
				m_rec_begin_syssec = 0;
				pthread_mutex_unlock(&m_mutex);	
			}else{
				usleep(20*1000);
			}

			break;
		case E_AACENCSTATE_DEINIT:
			pthread_mutex_lock(&m_mutex);

			if(paac){
				delete paac;paac = NULL;
			}
			m_aacencstate = E_AACENCSTATE_END;

			pthread_mutex_unlock(&m_mutex);
			break;

		case E_AACENCSTATE_END:
			//cout<<" E_AACENCSTATE_END "<<endl;
			pthread_mutex_lock(&m_mutex);
			m_aacencstate = E_AACENCSTATE_IDLE;
			pthread_mutex_unlock(&m_mutex);
			break;

		default:
			cout<<__FILE__<<" "<<__FUNCTION__<<" can't here "<<endl;
			sleep(1);exit(-1);
			break;
		}

		usleep(5*1000);

	}

	if(paac){
		delete paac;paac = NULL;
	}
}

////return 0 aac busy    1 can get aac
int pcm_aac::enc_cmd(time_t sec){
	int iret = 0;
	pthread_mutex_lock(&m_mutex);
	if(m_aacencstate == E_AACENCSTATE_IDLE || m_aacencstate == E_AACENCSTATE_END){
		m_aacencstate = E_AACENCSTATE_CMD;
		m_rec_begin_syssec = sec;
		iret = 1;
	}
	pthread_mutex_unlock(&m_mutex);
	return iret;
}

////-1 NO AAC    0 WAIT  1 0K
int pcm_aac::getaac(int sec,int &input_samples,std::vector<char>&v_decoder_info,std::deque<vector<char>>&dqv_aac){
	int iret ;
	pthread_mutex_lock(&m_mutex);
	if(m_rec_begin_syssec != sec){
		cout << __FILE__<<" "<<__FUNCTION__<<" m_rec_begin_syssec != sec "<< m_rec_begin_syssec<< " " << sec<<endl;
		iret = -1;
	}else if(m_aacencstate != E_AACENCSTATE_AACOK){
		iret = 0;
	}else{
		m_read_times ++;
		input_samples = m_aacinput_samples;
		v_decoder_info=mv_decoder_info;
		dqv_aac = m_aacque;
		iret = 1;
	}
	pthread_mutex_unlock(&m_mutex);

	return iret;
}


void tst_pcmaac(void){
	pcm_aac pcmaac(22050,1,16,10);
	pcmaac.init();

	while(1){
		//sleep(10);
		pcmaac.enc_cmd(syssec()-5);
		sleep(20);
	}
}

