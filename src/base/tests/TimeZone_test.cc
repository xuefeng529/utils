#include "base/TimeZone.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

using base::TimeZone;

void testShanghai()
{
	base::TimeZone shanghai("/usr/share/zoneinfo/Asia/Shanghai");
	for (int i = 0; i < 10; i++)
	{
		time_t now = time(NULL);
		struct tm tm;
		if (shanghai.valid())
		{
			printf("valid\n");
			tm = shanghai.toLocalTime(now);
		}
		else
		{
			gmtime_r(&now, &tm);
		}
		char timebuf[32];
		strftime(timebuf, sizeof timebuf, "%Y%m%d-%H%M%S", &tm);
		printf("%s\n", timebuf);
		sleep(3);
	}
}

int main()
{
  testShanghai();
}
