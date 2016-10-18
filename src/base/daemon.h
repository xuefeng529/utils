#ifndef BASE_DAEMON_H
#define BASE_DAEMON_H

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace base
{

void regfilelock(int lockfile)
{
	struct flock fl;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	fl.l_type = F_WRLCK;
	fl.l_pid = getpid();

	if (fcntl(lockfile, F_SETLKW, &fl) < 0)
	{
		perror("fcntl_reg");
		abort();
	}

	if (ftruncate(lockfile, 0) < 0)
	{
		perror("ftruncate");
		return;
	}
	if (lseek(lockfile, 0, SEEK_SET) < 0)
	{
		perror("seek");
		return;
	}
	__pid_t pid = getpid();
	char szpid[10] = { 0 };
	sprintf(szpid, "%d", pid);
	write(lockfile, szpid, strlen(szpid));
}

int checkrunning(int lockfile)
{
	struct flock fl;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	fl.l_type = F_WRLCK;

	if (fcntl(lockfile, F_GETLK, &fl) < 0)
	{
		perror("fcntl_get");
		exit(1);
	}

	if (fl.l_type == F_UNLCK)
	{
		regfilelock(lockfile);
		return 0;
	}
	return fl.l_pid;
}

int checkunique(const char* szlockfile)
{
	int lockfile = open(szlockfile, O_RDWR | O_CREAT, ACCESSPERMS);
	if (lockfile < 0)
	{
		perror("open lock file");
		exit(1);
	}

	return checkrunning(lockfile);
}

void daemonize(const char* pidfile)
{
	int lockfile = open(pidfile, O_RDWR | O_CREAT, ACCESSPERMS);
	if (lockfile < 0)
	{
		perror("Lockfile");
		abort();
	}

	int mun;
	if ((mun = checkrunning(lockfile)))
	{
		printf("Instance with pid %d running, just exit!\n", mun);
		abort();
	}

	pid_t pid;
	if ((pid = fork()) < 0)
	{
		perror("Fork\n");
		abort();
	}
	
	if (pid)
	{
		fprintf(stdout, "Info: Forked background with PID: [%d]\n", pid);
		exit(0);
	}

	setsid();        /* become session leader */
	chdir("/");      /* change working directory */
	umask(0);        /* clear our file mode creation mask */

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	regfilelock(lockfile);
}

} // namespace base

#endif // BASE_DAEMON_H
