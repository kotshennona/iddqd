/*
 * TODO: Insert a copyright notice.
 *
 * iddqd - Input Device Description Query Daemon
 *
 */

#define LOG_TAG "iddqd"
#define SOCKET_NAME "inputdevinfo_socket"

// NB that MAX_TOKEN_LENGTH and MAX_TOKEN_LENGTH macros are supposed
// to take into account string termination characher.
#define MAX_TOKEN_LENGTH 23
#define MAX_COMMAND_LENGTH 27

#define COMM_FILE_OPEN_MODE "a+"
#define TOKEN_SEPARATORS  " \t\n"

#include <cutils/sockets.h>
#include <cutils/log.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>

typedef struct{
	char cmd[MAX_TOKEN_LENGTH];
	char arg[MAX_TOKEN_LENGTH];
} iddqd_cmd;


/*
 * Function: parse_cmd
 * ----------------------
 * Parses a string to get a iddqd daemon command.
 *
 * n1: a reference to a char array
 * n2: a reference to a iddqd_cmd structure
 *
 * returns: 0 on success, 1 on error
 *
 */

int parse_cmd(char *string, iddqd_cmd *res){
	char *n_token; 


	n_token = strtok(string, TOKEN_SEPARATORS);

	if(n_token == NULL || (strlen(n_token) == 0) || (strlen(n_token) >= MAX_TOKEN_LENGTH)){
		return 1;	
	}

	strcpy (res->cmd, n_token);	
	
	// This would be a loop if we had variable number of argument.
	n_token = strtok(NULL, TOKEN_SEPARATORS);

	if(n_token == NULL || (strlen(n_token) == 0) || (strlen(n_token) >= MAX_TOKEN_LENGTH)){
		return 1;	
	}

	strcpy (res->arg, n_token);	

	return 0;
}

/*
 * Function: process_cmds
 * ----------------------
 * Opens a IO stream and reads all strings from it one by one;
 *
 * n1: a file descriptor 
 *
 * returns: always 0
 *
 */

static int process_cmds(int fd, int max_cmd_length){
	FILE *f;
	char cmd[MAX_COMMAND_LENGTH];
	iddqd_cmd pcmd;// p stands for "parsed"
	
	f = fdopen (fd, COMM_FILE_OPEN_MODE);
	if (f == NULL){
		ALOGE("Unable to open io stream (%s)\n", strerror(errno));
		return -1;
	}
	
	while (fgets(cmd, MAX_COMMAND_LENGTH - 1, f) != NULL){
		ALOGI("%s\n", cmd);
		if (!parse_cmd(cmd, &pcmd)){
			ALOGI("Command is %s\n", pcmd.cmd);
			ALOGI("Argument is %s\n", pcmd.arg);
		}
	// TODO: Clear the array
	}

	// TODO: Process return code	
	fclose (f);
	
	return 0;
}

/*
 * Function: serve_client
 * ----------------------
 * Reads input from a given file descriptor until this fd becomes invalid.
 * This usually happens when the other side terminates the connection.
 *
 * n1: a file descriptor 
 *
 * returns: always 0
 *
 */
static int serve_client(int fd){

	int current_flags;
	fd_set read_fds;

	//Adding O_NONBLOCK flag to the descriptor
	current_flags = fcntl(fd, F_GETFD);
	current_flags |= O_NONBLOCK;
	fcntl(fd, F_SETFD, current_flags);

	while(1) {
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);
		
		int retval = select(fd + 1, &read_fds, NULL, NULL, NULL);

		if(retval < 0){
			// Should I check for EBADR?
			break;
		} else if (retval > 0 && FD_ISSET(fd, &read_fds)) {
			ALOGI("We have some data to read from our only socket\n");
			process_cmds(fd, MAX_COMMAND_LENGTH);

		} else {
			;
		}
		
	}
	return 0;
}

int main() {

	int socket_fd;

	socket_fd = android_get_control_socket(SOCKET_NAME);
	
	if (socket_fd < 0 ){
		ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
		return -1;
	}

	if (listen (socket_fd, 1) < 0) {
		ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
		return -1;
	}

	while(1){
		int comm_socket_fd = accept(socket_fd, NULL, NULL);
		serve_client(comm_socket_fd);
	}

return 0;

}
