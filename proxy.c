#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	/* Force time zone to avoid having the TAS desync due to time zone. */
	setenv("TZ", "America/Chicago", 1);

	/* Deltarune only has one segment. If we are not on frame 0, start Undertale. */
	if (strcmp(getenv("LIBTAS_START_FRAME"), "0") != 0)
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
