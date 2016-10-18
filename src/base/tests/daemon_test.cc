#include "base/daemon.h"

#include <unistd.h>

int main(int argc, char* argv[])
{
	base::daemonize(argv[1]);
	while (1)
	{
		sleep(1);
	}
}