#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

int missing_daemon(int nochdir, int noclose)
{
    pid_t pid = 0;
    pid_t sid = 0;

    pid = fork();
    if (pid < 0) {
	    return -1;
    }

    if (pid > 0) {
	exit(0);
    }

    umask(0);

    sid = setsid();
    if (sid < 0) {
	return -1;
    }

    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid == -1) {
	return -1;
    } else if (pid != 0) {
	exit(0);
    }

    if (nochdir == 0) {
	chdir("/");
    }

    if (noclose == 0) {
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	if (open("/dev/null", O_RDONLY) == -1) {
	    return -1;
	}
	if (open("/dev/null", O_WRONLY) == -1) {
	    return -1;
	}
	if (open("/dev/null", O_RDWR) == -1) {
	    return -1;
	}
    }
    return (0);
}
