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
int do_libtas_handshake_program(int program_fd);
int do_libtas_handshake_game(int game_fd);
int proxy_communications(int program_fd, int game_fd);

struct
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

struct
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

typedef struct
{
	size_t length;
	char *data;
} string_t;

static bool user_quit = false;
static string_t dumpfile = {0, NULL};
static string_t ffmpegoptions = {0, NULL};
static int base_savestate_index;
static string_t base_savestate_path = {0, NULL};
static unsigned int base_frame_count = 0;
static unsigned int last_frame_count = 0;
static struct timespec last_frame_time = {0, 0};

int recvString(int fd, string_t *str)
{
	str->length = 0;
	free(str->data);
	str->data = NULL;

	if (recv(fd, &str->length, sizeof(str->length), MSG_WAITALL) != sizeof(str->length))
	{
		perror("failed to read string length");
		return 1;
	}
	str->data = calloc(1, str->length);
	if (!str->data)
	{
		perror("failed to allocate string");
		return 1;
	}
	if (recv(fd, str->data, str->length, MSG_WAITALL) != str->length)
	{
		perror("failed to read string");
		free(str->data);
		str->data = NULL;
		return 1;
	}

	return 0;
}

int sendString(int fd, string_t str)
{
	if (send(fd, &str.length, sizeof(str.length), 0) != sizeof(str.length))
	{
		perror("failed to write string length");
		return 1;
	}
	if (send(fd, str.data, str.length, 0) != str.length)
	{
		perror("failed to write string");
		return 1;
	}

	return 0;
}

int expectMessage(int fd, int expected)
{
	int message;
	if (recv(fd, &message, sizeof(message), MSG_WAITALL) != sizeof(message))
	{
		perror("failed to read message number");
		return 1;
	}
	if (message != expected)
	{
		fprintf(stderr, "unexpected message number; expecting %d but got %d\n", expected, message);
		return 1;
	}
	return 0;
}

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

	if (do_libtas_handshake_program(program_socket))
	{
		/* error already printed by do_libtas_handshake_program */
		return 1;
	}

	while (!user_quit)
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
			execv(argv[1], argv + 1);

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

		if (do_libtas_handshake_game(game_socket))
		{
			/* error already printed by do_libtas_handshake_game */
			return 1;
		}

		base_frame_count = last_frame_count;

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

	execl("/bin/rm", "rm", "-rf", tempDir, NULL);

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

int do_libtas_handshake_program(int program_fd)
{
#pragma pack(push, 1)
	struct
	{
		int msg_pid;
		pid_t pid;
		int msg_end;
	} payload;
#pragma pack(pop)
	payload.msg_pid = MSGB_PID;
	payload.pid = getpid();
	payload.msg_end = MSGB_END_INIT;

	if (send(program_fd, &payload, sizeof(payload), 0) != sizeof(payload))
	{
		perror("failed to send init packets to program");
		return 1;
	}

	if (expectMessage(program_fd, MSGN_CONFIG))
	{
		return 1;
	}

	if (recv(program_fd, &SharedConfig, sizeof(SharedConfig), MSG_WAITALL) != sizeof(SharedConfig))
	{
		perror("failed to read SharedConfig init packet from program");
		return 1;
	}

	SharedConfig.prevent_savefiles = false;
	last_frame_time = SharedConfig.initial_time;

	if (SharedConfig.av_dumping)
	{
		if (expectMessage(program_fd, MSGN_DUMP_FILE))
		{
			return 1;
		}

		if (recvString(program_fd, &dumpfile))
		{
			return 1;
		}

		if (recvString(program_fd, &ffmpegoptions))
		{
			return 1;
		}
	}

	if (SharedConfig.incremental_savestates)
	{
		if (expectMessage(program_fd, MSGN_BASE_SAVESTATE_INDEX))
		{
			return 1;
		}

		if (recv(program_fd, &base_savestate_index, sizeof(base_savestate_index), MSG_WAITALL) != sizeof(base_savestate_index))
		{
			perror("failed to read base savestate index");
			return 1;
		}

		if (!SharedConfig.savestates_in_ram) {
			if (expectMessage(program_fd, MSGN_BASE_SAVESTATE_PATH))
			{
				return 1;
			}

			if (recvString(program_fd, &base_savestate_path))
			{
				return 1;
			}
		}
	}

	return expectMessage(program_fd, MSGN_END_INIT);
}

int do_libtas_handshake_game(int game_fd)
{
	if (expectMessage(game_fd, MSGB_PID))
	{
		return 1;
	}

	pid_t pid;
	if (recv(game_fd, &pid, sizeof(pid), MSG_WAITALL) != sizeof(pid))
	{
		perror("failed to read PID from game");
		return 1;
	}

	if (expectMessage(game_fd, MSGB_END_INIT))
	{
		return 1;
	}

	int message = MSGN_CONFIG;
	if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
	{
		perror("failed to send MSGN_CONFIG init packet to game");
		return 1;
	}

	if (send(game_fd, &SharedConfig, sizeof(SharedConfig), MSG_WAITALL) != sizeof(SharedConfig))
	{
		perror("failed to send SharedConfig init packet to game");
		return 1;
	}

	if (SharedConfig.av_dumping)
	{
		message = MSGN_DUMP_FILE;
		if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
		{
			perror("failed to send MSGN_DUMP_FILE init packet to game");
			return 1;
		}

		if (sendString(game_fd, dumpfile))
		{
			return 1;
		}

		if (sendString(game_fd, ffmpegoptions))
		{
			return 1;
		}
	}

	if (SharedConfig.incremental_savestates)
	{
		message = MSGN_BASE_SAVESTATE_INDEX;
		if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
		{
			perror("failed to send MSGN_BASE_SAVESTATE_INDEX init packet to game");
			return 1;
		}

		if (send(game_fd, &base_savestate_index, sizeof(base_savestate_index), 0) != sizeof(base_savestate_index))
		{
			perror("failed to send base_savestate_index to game");
			return 1;
		}

		if (!SharedConfig.savestates_in_ram) {
			message = MSGN_BASE_SAVESTATE_PATH;
			if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
			{
				perror("failed to send MSGN_BASE_SAVESTATE_PATH init packet to game");
				return 1;
			}

			if (sendString(game_fd, base_savestate_path))
			{
				return 1;
			}
		}
	}

	message = MSGN_END_INIT;
	if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
	{
		perror("failed to send MSGN_END_INIT packet to game");
		return 1;
	}

	return 0;
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

static bool gameEndInit = false;

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

	if (send(game_fd, &message, sizeof(message), 0) != sizeof(message))
	{
		perror("failed to send message to game");
		return 1;
	}

	string_t str = {0, NULL};
	string_t *pstr = &str;
	int i = 0;
	int *pi = &i;

	switch (message)
	{
		case MSGN_USERQUIT:
			user_quit = true;
			/* fallthrough */
		case MSGN_START_FRAMEBOUNDARY:
		case MSGN_END_FRAMEBOUNDARY:
		case MSGN_SAVESTATE:
		case MSGN_LOADSTATE:
		case MSGN_STOP_ENCODE:
		case MSGN_EXPOSE:
			break;
		case MSGN_ALL_INPUTS:
		case MSGN_PREVIEW_INPUTS:
			if (recv(program_fd, &AllInputs, sizeof(AllInputs), MSG_WAITALL) != sizeof(AllInputs))
			{
				perror("failed to read AllInputs from the program");
				return 1;
			}
			if (send(game_fd, &AllInputs, sizeof(AllInputs), 0) != sizeof(AllInputs))
			{
				perror("failed to send AllInputs to the game");
				return 1;
			}
			break;
		case MSGN_CONFIG:
			if (recv(program_fd, &SharedConfig, sizeof(SharedConfig), MSG_WAITALL) != sizeof(SharedConfig))
			{
				perror("failed to read SharedConfig from the program");
				return 1;
			}
			SharedConfig.prevent_savefiles = false;
			SharedConfig.initial_time = last_frame_time;
			if (send(game_fd, &SharedConfig, sizeof(SharedConfig), 0) != sizeof(SharedConfig))
			{
				perror("failed to send SharedConfig to the game");
				return 1;
			}
			break;
		case MSGN_DUMP_FILE:
			if (recvString(program_fd, &dumpfile))
			{
				return 1;
			}
			if (recvString(program_fd, &ffmpegoptions))
			{
				return 1;
			}
			if (sendString(game_fd, dumpfile))
			{
				return 1;
			}
			if (sendString(game_fd, ffmpegoptions))
			{
				return 1;
			}
			break;
		case MSGN_BASE_SAVESTATE_PATH:
			pstr = &base_savestate_path;
			/* fallthrough */
		case MSGN_OSD_MSG:
		case MSGN_SAVESTATE_PATH:
		case MSGN_PARENT_SAVESTATE_PATH:
		case MSGN_RAMWATCH:
			if (recvString(program_fd, pstr))
			{
				return 1;
			}
			i = sendString(game_fd, *pstr);
			free(str.data);
			str.data = NULL;
			if (i)
			{
				return 1;
			}
			break;
		case MSGN_BASE_SAVESTATE_INDEX:
			pi = &base_savestate_index;
			/* fallthrough */
		case MSGN_SAVESTATE_INDEX:
		case MSGN_PARENT_SAVESTATE_INDEX:
			if (recv(program_fd, pi, sizeof(*pi), MSG_WAITALL) != sizeof(*pi))
			{
				perror("failed to read int");
				return 1;
			}
			if (send(game_fd, pi, sizeof(*pi), 0) != sizeof(*pi))
			{
				perror("failed to send int");
				return 1;
			}
			break;
		default:
			fprintf(stderr, "unexpected message from program: %d\n", message);
			return 1;
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

#pragma pack(push, 1)
	struct
	{
		unsigned long framecount;
		struct timespec time;
	} framecount_time;
#pragma pack(pop)
	pid_t pid;
	int window;
	GameInfo gi;
	float fps[2];
	string_t str;

	size_t payload_size = 0;
	void *payload = NULL;
	bool read_payload = false;

	switch (message)
	{
		case MSGB_QUIT:
			if (!user_quit)
			{
				/* don't forward */
				return 0;
			}
			/* fallthrough */
		case MSGB_START_FRAMEBOUNDARY:
		case MSGB_LOADING_SUCCEEDED:
		case MSGB_ENCODE_FAILED:
			break;
		case MSGB_FRAMECOUNT_TIME:
			if (recv(game_fd, &framecount_time, sizeof(framecount_time), MSG_WAITALL) != sizeof(framecount_time))
			{
				perror("failed to read framecount_time packet");
				return 1;
			}
			framecount_time.framecount += base_frame_count;
			last_frame_count = framecount_time.framecount;
			last_frame_time = framecount_time.time;
			payload = &framecount_time;
			payload_size = sizeof(framecount_time);
			break;
		case MSGB_WINDOW_ID:
			payload = &window;
			payload_size = sizeof(window);
			read_payload = true;
			break;
		case MSGB_ALERT_MSG:
			if (recvString(game_fd, &str))
			{
				return 1;
			}
			if (send(program_fd, &message, sizeof(message), 0) != sizeof(message))
			{
				perror("failed to send message to program");
				return 1;
			}
			if (sendString(program_fd, str))
			{
				return 1;
			}
			/* don't send the payload using the shared path */
			return 0;
		case MSGB_GAMEINFO:
			payload = &gi;
			payload_size = sizeof(gi);
			read_payload = true;
			break;
		case MSGB_FPS:
			payload = &fps;
			payload_size = sizeof(fps);
			read_payload = true;
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

	if (read_payload)
	{
		if (recv(game_fd, payload, payload_size, MSG_WAITALL) != payload_size)
		{
			perror("failed to read packet from game");
			return 1;
		}
	}

	if (payload_size)
	{
		if (send(program_fd, payload, payload_size, 0) != payload_size)
		{
			perror("failed to send packet to program");
			return 1;
		}
	}

	return 0;
}
