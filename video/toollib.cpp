
#include <unistd.h>
#include <sys/sysinfo.h>

#include "toollib.h"

int syssec(void)
{
	struct sysinfo s_info;
	sysinfo(&s_info);
	return s_info.uptime;
}
