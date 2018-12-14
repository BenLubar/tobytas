#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fputs("missing program name", stderr);
		return 1;
	}

	for (;;)
	{
		pid_t pid = fork();
		if (pid < 0)
		{
			/* error! */
			perror("failed to fork");
			return 1;
		}

		if (!pid)
		{
			/* we are the child; execute the process */
			execv(argv[1], argv + 2);

			/* if we got this far, there's an error */
			perror("failed to exec");
			return 1;
		}

		/* wait for the process to exit */
		int wstatus;
		if (waitpid(pid, &wstatus, 0) == -1)
		{
			perror("failed to wait");
			return 1;
		}

		if (!WIFEXITED(wstatus))
		{
			fputs("did not exit normally; must have crashed", stderr);
			return 1;
		}

		if  (WEXITSTATUS(wstatus) != 0)
		{
			fprintf(stderr, "exited with code %d; stopping\n", WEXITSTATUS(wstatus));
			return 1;
		}

		/* there's a reason I'm doing this even though it seems redundant */
		if (chdir("../undertale"))
		{
			perror("failed to change directory");
			return 1;
		}
		static char exe[] = "./undertale-runner";
		argv[1] = exe;
	}

	return 0;
}
