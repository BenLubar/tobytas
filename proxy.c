#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "messages.h"

#define SOCKET_FILENAME "/tmp/libTAS.socket"

void remove_temp_directory(void);
int init_program_socket(void);
int init_game_socket(void);
int proxy_communications(int program_fd, int game_fd);

typedef struct
{
	unsigned long int keys[16];
	int pointer_x, pointer_y;
	unsigned int pointer_mask;
	short controller_axes[4][6];
	unsigned short controller_buttons[4];
} AllInputs;

enum FastForwardMode
{
	FF_SLEEP = 0x01,
	FF_MIXING = 0x02,
	FF_RENDERING = 0x04
};

enum RecStatus
{
	NO_RECORDING,
	RECORDING_WRITE,
	RECORDING_READ
};

enum OSDFlags
{
	OSD_FRAMECOUNT = 0x01,
	OSD_INPUTS = 0x02,
	OSD_MESSAGES = 0x04,
	OSD_RAMWATCHES = 0x08
};

enum OSDLocation
{
	OSD_LEFT = 0x01,
	OSD_HCENTER = 0x02,
	OSD_RIGHT = 0x04,
	OSD_TOP = 0x10,
	OSD_VCENTER = 0x20,
	OSD_BOTTOM = 0x40
};

enum TimeCallType
{
	TIMETYPE_UNTRACKED = -1,
	TIMETYPE_TIME = 0,
	TIMETYPE_GETTIMEOFDAY,
	TIMETYPE_CLOCK,
	TIMETYPE_CLOCKGETTIME,
	TIMETYPE_SDLGETTICKS,
	TIMETYPE_SDLGETPERFORMANCECOUNTER,
	TIMETYPE_NUMTRACKEDTYPES
};

enum DebugFlags
{
	DEBUG_UNCONTROLLED_TIME = 0x01,
	DEBUG_NATIVE_EVENTS = 0x02
};

typedef int LogCategoryFlag;

typedef struct
{
	bool running;
	int speed_divisor;
	bool fastforward;
	int fastforward_mode;
	int recording;
	unsigned long movie_framecount;
	int logging_status;
	LogCategoryFlag includeFlags;
	LogCategoryFlag excludeFlags;
	bool av_dumping;
	unsigned int framerate_num;
	unsigned int framerate_den;
	bool keyboard_support;
	bool mouse_support;
	int nb_controllers;
	int osd;
	bool osd_encode;
	int osd_frame_location;
	int osd_inputs_location;
	int osd_messages_location;
	int osd_ramwatches_location;
	bool prevent_savefiles;
	int audio_bitdepth;
	int audio_channels;
	int audio_frequency;
	bool audio_mute;
	int video_codec;
	int video_bitrate;
	int audio_codec;
	int audio_bitrate;
	int main_gettimes_threshold[TIMETYPE_NUMTRACKEDTYPES];
	int sec_gettimes_threshold[TIMETYPE_NUMTRACKEDTYPES];
	bool save_screenpixels;
	struct timespec initial_time;
	int screen_width;
	int screen_height;
	bool incremental_savestates;
	bool savestates_in_ram;
	int debug_state;
	bool recycle_threads;
	int locale;
	bool virtual_steam;
} SharedConfig;

typedef enum
{
	NONE,
	CORE,
	COMPATIBILITY,
	ES
} GameInfoProfile;

typedef struct
{
	bool tosend;

	int video;
	int audio;
	int keyboard;
	int mouse;
	int joystick;

	int opengl_major;
	int opengl_minor;

	GameInfoProfile opengl_profile;
} GameInfo;

static char tempDir[] = "/tmp/tasFiles.XXXXXX";

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		fputs("missing program name\n", stderr);
		return 1;
	}

	if (!mkdtemp(tempDir))
	{
		perror("could not create temporary directory");
		return 1;
	}

	if (atexit(&remove_temp_directory))
	{
		perror("could not queue deletion of temporary directory");
		return 1;
	}

	const int program_socket = init_program_socket();
	if (program_socket < 0)
	{
		/* error already printed by init_program_socket */
		return 1;
	}

	for (;;)
	{
		const pid_t pid = fork();
		if (pid < 0)
		{
			/* error! */
			perror("failed to fork");
			return 1;
		}

		if (!pid)
		{
			/* set HOME to the temporary save directory */
			setenv("HOME", tempDir, 1);

			/* prevent the child process from deleting the save directory if exec fails */
			tempDir[0] = 0;

			/* we are the child; execute the process */
			execv(argv[1], argv + 2);

			/* if we got this far, there's an error */
			perror("failed to exec");
			return 1;
		}

		const int game_socket = init_game_socket();
		if (game_socket < 0)
		{
			/* error already printed by init_game_socket */
			return 1;
		}

		if (proxy_communications(program_socket, game_socket))
		{
			/* error already printed by proxy_communications */
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
			fputs("did not exit normally; must have crashed\n", stderr);
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

void remove_temp_directory()
{
	if (!tempDir[0])
	{
		return;
	}

	execl("/bin/rm", "-rf", tempDir, NULL);

	/* if execl failed, we will reach this code */
	tempDir[0] = 0;
	perror("failed to remove temporary directory");
}

int init_program_socket(void)
{
	struct stat st;
	if (!stat(SOCKET_FILENAME, &st))
	{
		fputs("socket already created by another process; multiple games started too close together?\n", stderr);
		return -1;
	}

	const struct sockaddr_un addr = { AF_UNIX, SOCKET_FILENAME };
	const int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		perror("could not allocate socket");
		return -1;
	}

	if (bind(listen_fd, (const struct sockaddr *)(&addr), sizeof(struct sockaddr_un)))
	{
		perror("failed to bind socket");
		close(listen_fd);
		return -1;
	}

	if (listen(listen_fd, 1))
	{
		perror("failed to listen for the program");
		close(listen_fd);
		return -1;
	}

	const int socket_fd = accept(listen_fd, NULL, NULL);
	if (socket_fd < 0)
	{
		perror("failed to accept connection");
		close(listen_fd);
		return -1;
	}

	close(listen_fd);
	unlink(SOCKET_FILENAME);

	return socket_fd;
}

int init_game_socket(void)
{
	const struct sockaddr_un addr = { AF_UNIX, SOCKET_FILENAME };
	const int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	usleep(15000);
	while (connect(socket_fd, (const struct sockaddr *)(&addr), sizeof(struct sockaddr_un)))
	{
		if (errno != ENOENT)
		{
			perror("failed to connect to game");
			close(socket_fd);
			return -1;
		}

		usleep(15000);
	}

	unlink(SOCKET_FILENAME);

	return socket_fd;
}

int handle_program_message(int program_fd, int game_fd);
int handle_game_message(int program_fd, int game_fd);

int proxy_communications(int program_fd, int game_fd)
{
	fd_set fd_none;
	fd_set fd_wait;
	int maxfd = program_fd > game_fd ? program_fd : game_fd;
	maxfd++;

	FD_ZERO(&fd_none);

	for (;;)
	{
		FD_ZERO(&fd_wait);
		FD_SET(program_fd, &fd_wait);
		FD_SET(game_fd, &fd_wait);

		int ready = select(maxfd, &fd_wait, &fd_none, &fd_none, NULL);

		if (ready <= 0)
		{
			perror("failed to receive data over the socket");
			return 1;
		}

		if (FD_ISSET(program_fd, &fd_wait))
		{
			if (handle_program_message(program_fd, game_fd))
			{
				return 1;
			}
		}

		if (FD_ISSET(game_fd, &fd_wait))
		{
			if (handle_game_message(program_fd, game_fd))
			{
				return 1;
			}
		}
	}
}

int handle_program_message(int program_fd, int game_fd)
{
	int message;
	if (recv(program_fd, &message, sizeof(message), MSG_WAITALL) != sizeof(message))
	{
		perror("failed to read message from program");
		return 1;
	}

	void *payload = NULL;
	size_t payload_size = 0;

	switch (message)
	{
		case MSGN_START_FRAMEBOUNDARY:
		case MSGN_END_FRAMEBOUNDARY:
		case MSGN_USERQUIT:
		case MSGN_END_INIT:
		case MSGN_SAVESTATE:
		case MSGN_LOADSTATE:
		case MSGN_STOP_ENCODE:
		case MSGN_EXPOSE:
			break;
		case MSGN_ALL_INPUTS:
		case MSGN_PREVIEW_INPUTS:
			payload = calloc(1, sizeof(AllInputs));
			if (!payload)
			{
				perror("failed to allocate AllInputs");
				return 1;
			}
			payload_size = sizeof(AllInputs);
			if (recv(program_fd, payload, payload_size, MSG_WAITALL) != payload_size)
			{
				perror("failed to read AllInputs from the program");
				free(payload);
				return 1;
			}
			break;
		case MSGN_CONFIG:
			payload = calloc(1, sizeof(SharedConfig));
			if (!payload)
			{
				perror("failed to allocate SharedConfig");
				return 1;
			}
			payload_size = sizeof(SharedConfig);
			if (recv(program_fd, payload, payload_size, MSG_WAITALL) != payload_size)
			{
				perror("failed to read SharedConfig from the program");
				free(payload);
				return 1;
			}
			((SharedConfig *)payload)->prevent_savefiles = false;
			break;
		case MSGN_DUMP_FILE:
		case MSGN_OSD_MSG:
		case MSGN_SAVESTATE_PATH:
		case MSGN_BASE_SAVESTATE_PATH:
		case MSGN_PARENT_SAVESTATE_PATH:
		case MSGN_RAMWATCH:
			if (recv(program_fd, &payload_size, sizeof(payload_size), MSG_WAITALL | MSG_PEEK) != sizeof(payload_size))
			{
				perror("failed to read string length from the program");
				return 1;
			}
			payload_size += sizeof(payload_size);
			payload = calloc(1, payload_size);
			if (!payload)
			{
				perror("failed to allocate string");
				return 1;
			}
			if (recv(program_fd, payload, payload_size, MSG_WAITALL) != payload_size)
			{
				perror("failed to read string");
				free(payload);
				return 1;
			}
			break;
		case MSGN_SAVESTATE_INDEX:
		case MSGN_PARENT_SAVESTATE_INDEX:
		case MSGN_BASE_SAVESTATE_INDEX:
			payload_size = sizeof(int);
			payload = calloc(1, sizeof(int));
			if (!payload)
			{
				perror("failed to allocate int");
				return 1;
			}
			if (recv(program_fd, payload, payload_size, MSG_WAITALL) != payload_size)
			{
				perror("failed to read int");
				free(payload);
				return 1;
			}
			break;
		default:
			fprintf(stderr, "unexpected message from program: %d\n", message);
			return 1;
	}

	if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
	{
		perror("failed to send message to game");
		free(payload);
		return 1;
	}

	if (payload_size)
	{
		if (send(game_fd, payload, payload_size, 0) != payload_size)
		{
			perror("failed to send payload to game");
			free(payload);
			return 1;
		}

		free(payload);
	}

	return 0;
}

int handle_game_message(int program_fd, int game_fd)
{
	int message;
	if (recv(game_fd, &message, sizeof(message), MSG_WAITALL) != sizeof(message))
	{
		perror("failed to read message from game");
		return 1;
	}

	size_t payload_size = 0;
	size_t cstring = 0;

	switch (message)
	{
		case MSGB_START_FRAMEBOUNDARY:
		case MSGB_QUIT:
		case MSGB_END_INIT:
		case MSGB_LOADING_SUCCEEDED:
		case MSGB_ENCODE_FAILED:
			break;
		case MSGB_FRAMECOUNT_TIME:
			payload_size = sizeof(unsigned long) + sizeof(struct timespec);
			break;
		case MSGB_PID:
			payload_size = sizeof(pid_t);
			break;
		case MSGB_WINDOW_ID:
			payload_size = sizeof(int);
			break;
		case MSGB_ALERT_MSG:
			cstring = 1;
			break;
		case MSGB_GAMEINFO:
			payload_size = sizeof(GameInfo);
			break;
		case MSGB_FPS:
			payload_size = sizeof(float) * 2;
			break;
		default:
			fprintf(stderr, "unexpected message from game: %d\n", message);
			return 1;
	}

	if (send(program_fd, &message, sizeof(message), 0) != sizeof(message))
	{
		perror("failed to send message to program");
		return 1;
	}

	void *payload = NULL;
	if (payload_size)
	{
		payload = calloc(1, payload_size);
		if (!payload)
		{
			perror("failed to allocate space for game packet");
			return 1;
		}

		if (recv(game_fd, payload, payload_size, MSG_WAITALL) != payload_size)
		{
			perror("failed to read game payload");
			free(payload);
			return 1;
		}

		if (send(program_fd, payload, payload_size, 0) != payload_size)
		{
			perror("failed to send payload to program");
			free(payload);
			return 1;
		}

		free(payload);
	}

	for (; cstring; cstring--)
	{
		if (recv(game_fd, &payload_size, sizeof(payload_size), MSG_WAITALL) != sizeof(payload_size))
		{
			perror("failed to read string length from game");
			return 1;
		}

		if (send(program_fd, &payload_size, sizeof(payload_size), 0) != sizeof(payload_size))
		{
			perror("failed to send string length to program");
			return 1;
		}

		payload = calloc(1, payload_size);
		if (!payload)
		{
			perror("failed to allocate space for game string");
			return 1;
		}

		if (recv(game_fd, payload, payload_size, MSG_WAITALL) != payload_size)
		{
			perror("failed to read string from game");
			free(payload);
			return 1;
		}

		if (send(program_fd, payload, payload_size, 0) != payload_size)
		{
			perror("failed to send string to program");
			free(payload);
			return 1;
		}

		free(payload);
	}

	return 0;
}
