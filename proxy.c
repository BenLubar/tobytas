#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	/* Force time zone to avoid having the TAS desync due to time zone. */
	setenv("TZ", "America/Chicago", 1);

	/* Set the home directory to force save files to stay separate. */
	char fakehome[PATH_MAX];
	getcwd(fakehome, sizeof(fakehome));
	strcat(fakehome, "/.home");
	setenv("HOME", fakehome, 1);

	/* Deltarune only has one segment. If we are not on frame 0, start Undertale. */
	if (strcmp(getenv("LIBTAS_START_FRAME"), "0") != 0)
	{
		if (chdir("../undertale"))
		{
			perror("failed to chdir");
		}

		argv[1] = "./undertale-runner";
	}
	else
	{
		/* If we are on frame 0, delete the old fake home directory first. */
		pid_t pid = fork();
		if (pid)
		{
			int status;
			waitpid(pid, &status, 0);
		}
		else
		{
			execlp("rm", "rm", "-rf", fakehome, NULL);
		}
	}

	execv(argv[1], argv + 1);
	perror("failed to exec!");

	return -1;
}
