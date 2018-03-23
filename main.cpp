#include <time.h>

#include <iostream>
#include <vector>
using namespace std;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>

#include "yuv2rgb.h"
#include "rgb_bmp.h"

#include "cam_cap.h"
#include "VPUWraper.h"
#include "vpucls.h"
#include "cam_h264.h"
#include "video_mng.h"
#include "snd_pcm.h"
#include "pcm_aac.h"

int main(int argc,char*argv[]){
	//test_cam();
	//test_vpu();
	//test_vpu3();
	//tst_cam264();
	tst_vmng();
	//tst_pcm();
	//tst_pcmaac();
	return 0;
}


