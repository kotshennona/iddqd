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

#define MAX(a,b) ((a < b) ?  (b) : (a))

#include <cutils/sockets.h>
#include <cutils/log.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/ioctl.h>
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
	ALOGI("Returning from process_cmds");
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

	return 0;
}

int main() {

	int l_socket_fd = -1;
	int c_socket_fd = -1;
	int current_flags;
	fd_set fds;

	//Adding O_NONBLOCK flag to the descriptor
	current_flags = fcntl(l_socket_fd, F_GETFD);
	current_flags |= O_NONBLOCK;
	fcntl(l_socket_fd, F_SETFD, current_flags);

	l_socket_fd = android_get_control_socket(SOCKET_NAME);
	
	if (l_socket_fd < 0 ){
		ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
		return -1;
	}

	if (listen (l_socket_fd, 0) < 0) {
		ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
		return -1;
	}

	while(1){
		FD_ZERO(&fds);
		FD_SET(l_socket_fd, &fds);

		if (c_socket_fd >= 0 ) {
			// &&  fcntl(c_socket_fd, F_GETFD) >= 0 ? 
			FD_SET(c_socket_fd, &fds);
		}

		int retval = select(MAX(c_socket_fd, l_socket_fd) + 1, &fds, NULL, NULL, NULL);

		if(retval <= 0){
			ALOGI("Error\n");
			// Should I check for EBADR?
			break;
		} 

		if(FD_ISSET(l_socket_fd, &fds)) {
			ALOGI("Connection attempt\n");
			if(c_socket_fd < 0) {
				c_socket_fd = accept(l_socket_fd, NULL, NULL);

				//Adding O_NONBLOCK flag to the descriptor
				current_flags = fcntl(c_socket_fd, F_GETFD);
				current_flags |= O_NONBLOCK;
				fcntl(c_socket_fd, F_SETFD, current_flags);
			}
		} 

		if(c_socket_fd >= 0 && FD_ISSET(c_socket_fd, &fds)){

			int unread_bytes_count;
			if (ioctl(c_socket_fd, FIONREAD, &unread_bytes_count)){
				ALOGE("Attempt to check if client socket is closed resulted in error.\n");
			} else if(unread_bytes_count == 0) {
				//TODO: Check return code?				
				close(c_socket_fd);
				c_socket_fd = -1;
			} else {
				process_cmds(c_socket_fd, MAX_COMMAND_LENGTH);
				close(c_socket_fd);
				c_socket_fd = -1;
			}
		}

	}

return 0;

}
