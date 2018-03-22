
#include <unistd.h>
#include <sys/sysinfo.h>
#include<sys/time.h>
#include "toollib.h"

uint64_t syssec(void)
{
	struct sysinfo s_info;
	sysinfo(&s_info);
	return s_info.uptime;
}

uint64_t getms(void)
{
	uint64_t ms;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ms = tv.tv_sec;
	ms*=1000;
	ms+=tv.tv_usec/1000;
	return ms;
}

uint64_t getsec(void)
{
	return getms()/1000;
}
