#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *savedir = getenv("HOME");

	/* Force time zone to avoid having the TAS desync due to time zone. */
	setenv("TZ", "America/Chicago", 1);

	/* Deltarune only has one segment. If Deltarune has saved, assume we are on an Undertale segment. */
	char buf[PATH_MAX];
	snprintf(buf, PATH_MAX, "%s/.config/DELTARUNE", savedir);

	struct stat st;
	if (!stat(buf, &st))
	{
		if (chdir("../undertale"))
		{
			perror("failed to chdir");
		}

		argv[1] = "./undertale-runner";
	}

	execv(argv[1], argv + 1);
	perror("failed to exec!");

	return -1;
}
