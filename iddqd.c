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
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

typedef struct{
	char cmd[MAX_TOKEN_LENGTH];
	char arg[MAX_TOKEN_LENGTH];
} iddqd_cmd;

/*
 * Function: get_max
 * -------------------
 * Takes two int arguments and returns the largest one
 *
 * n1: integer
 * n2: integer
 *
 * returns: the largest of two given integers
 */
// Should I make this one inline?
int get_max (int first, int second) {
	return (first < second) ?  (second) : (first);
}

/*
 * Function: make_nonblocking 
 * Enables a non-blocking mode for a given file descriptor.
 * 
 * n1: int file descriptor
 *
 * returns: 0 on succes, 1 on error
 *
 */

int make_nonblocking (int fd) {
	int current_flags;
	current_flags = fcntl(fd, F_GETFD);
	
	if (current_flags < 0) {
		return 1;
	}
	current_flags |= O_NONBLOCK;
	
	if (fcntl(fd, F_SETFD, current_flags) < 0){
		return 1;
	}
	
	return 0;
}

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

int main() {

	int l_socket_fd = -1;
	int c_socket_fd = -1;
	fd_set fds;

	l_socket_fd = android_get_control_socket(SOCKET_NAME);

	if (l_socket_fd < 0 ){
		ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
		return -1;
	}

	if (make_nonblocking (l_socket_fd)) {
		ALOGE("Unable to modify inputdevinfo_socket flags. (%s)\n", strerror(errno));
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

		int retval = select(get_max(c_socket_fd, l_socket_fd) + 1, &fds, NULL, NULL, NULL);

		if(retval <= 0){
			ALOGI("Error\n");
			// Should I check for EBADR?
			break;
		} 

		if(FD_ISSET(l_socket_fd, &fds)) {
			ALOGI("Connection attempt\n");
			if(c_socket_fd < 0) {
				c_socket_fd = accept(l_socket_fd, NULL, NULL);
			}

			if (make_nonblocking (c_socket_fd)) {
				ALOGE("Unable to modify socket flags. (%s)\n", strerror(errno));
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
