#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	char buf[PATH_MAX];
	snprintf(buf, PATH_MAX, "%s/.config/DELTARUNE", getenv("HOME"));

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
