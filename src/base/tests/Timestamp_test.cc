#include "base/Timestamp.h"
#include <vector>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

using base::Timestamp;

void passByConstReference(const Timestamp& x)
{
	printf("%s\n", x.toString().c_str());
}

void passByValue(Timestamp x)
{
	printf("%s\n", x.toString().c_str());
}

int main()
{
	int cnt = 10;
	while (cnt--)
	{
		printf("now: %" PRId64 "\n", Timestamp::now().microSecondsSinceEpoch());
		sleep(1);
	}
	Timestamp now(Timestamp::now());
	printf("%s\n", now.toString().c_str());
	passByValue(now);
	passByConstReference(now);
}

