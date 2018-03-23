
#include <iostream>
using namespace std;

#include "frame_mng.h"





frame_mng::frame_mng(int t_sec){
	m_que_sec = t_sec;
	m_cursec = 0;
	pthread_mutex_init(&m_mutex, NULL);
}
frame_mng::~frame_mng(void){
	pthread_mutex_destroy(&m_mutex);
}

////return 0 no data sleep     1 have data
int frame_mng::get_sec_frame(int sec, std::vector<TYPE_VU8> &v_sec){
	if (sec <= 0){
		cerr << __FILE__ << " " << __FUNCTION__ << " sec<=0 sec:" << sec << endl;
		return -1;
	}

	pthread_mutex_lock(&m_mutex);
	map<int, vector<TYPE_SPVU8>>::iterator it;
	if(m_fmap.size() > 0){
		it = m_fmap.begin();
		if(sec<it->first){
			cerr << __FILE__ << " " << __FUNCTION__ << " req sec < min map sec " << sec << endl;
			pthread_mutex_unlock(&m_mutex);
			return -1;
		}
	}
	it = m_fmap.find(sec);
	if (it == m_fmap.end()){
		pthread_mutex_unlock(&m_mutex);
		return 0;
	}
	v_sec.clear();
	for (int i = 0; i < m_fmap[sec].size(); i++){
		v_sec.push_back(*m_fmap[sec][i]);
	}
	pthread_mutex_unlock(&m_mutex);
	return 1;
}

void frame_mng::add_frame(int sec, TYPE_SPVU8 frame){

	if (sec <= 0){
		cerr << __FILE__ << " " << __FUNCTION__ << " sec<=0 sec:" << sec << endl;
		return;
	}

	pthread_mutex_lock(&m_mutex);

	if (sec != m_cursec){
		if (m_cursec != 0){
			m_fmap[m_cursec] = mv_cursec;
			if ((int)m_fmap.size() > m_que_sec){
				m_fmap.erase(m_fmap.begin());
			}
			mv_cursec.clear();
		}
		m_cursec = sec;
	}
	mv_cursec.insert(mv_cursec.end(), frame);

	pthread_mutex_unlock(&m_mutex);
}



