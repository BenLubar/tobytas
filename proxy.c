#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *savedir = getenv("HOME");

	(void)mkdir(savedir, 0755);

	unsigned char segmentNumber = 0;
	char segment[4];

	char buf[PATH_MAX];
	snprintf(buf, PATH_MAX, "%s/.tas-segment", savedir);
	FILE *segmentFile = fopen(buf, "rb");
	if (segmentFile)
	{
		fread(&segmentNumber, 1, 1, segmentFile);
		fclose(segmentFile);
	}
	segmentNumber++;
	segmentFile = fopen(buf, "wb");
	fwrite(&segmentNumber, 1, 1, segmentFile);
	fclose(segmentFile);

	snprintf(segment, 4, "%d", segmentNumber);
	setenv("TAS_SEGMENT", segment, 1);

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
