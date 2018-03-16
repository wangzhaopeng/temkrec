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


int main(int argc,char*argv[])
{
	//test_cam();
	test_vpu();
	return 0;
}


