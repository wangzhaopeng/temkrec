#ifndef __MYMP4_H__
#define __MYMP4_H__

#include <string.h>
#include <vector>
#include <string>

class mymp4
{
public:
	mymp4(void);
	~mymp4(void);
	bool init(const char * pfile,int time_scale = 90000);

	//audio
	bool init_a(int hz, int m_input_samples, const std::vector<char>&v_aac_decoder_info);
	bool write_a(const char *pd, int size);

	//video
	bool init_v(int width,int height, int frameRate,const std::vector<char>&v_sps,const std::vector<char>&v_pps);
	bool write_v(const char *pd, int size);

private:
	void deinit(void);
	mymp4(const mymp4&);
	mymp4& operator=(const mymp4&);

	void* m_h_mp4;
	int m_time_scale;
	std::string m_file_name;

	//audio
	void deinit_a(void);
	unsigned int m_a_track_id; //MP4TrackId use unsigned int

	//video
	void deinit_v(void);
	unsigned int m_v_track_id;
	int m_v_width;
	int m_v_height;
	int m_v_frame_rate;
};

#endif
