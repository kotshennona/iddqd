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
#define TOKEN_SEPARATORS  " \t\n"
#define COMMAND_SEPARATOR "--"
#define COMMAND_SEPARATOR_LENGTH 2

#define MAX_READ_BUFFER_SIZE 64

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
 * Reads commands from file. Commands are separated by COMMAND_SEPARATOR.
 *
 * n1: a file descriptor 
 *
 * returns:  0 on succes, 1 on error
 *
 */

static int process_cmds(int fd, int max_cmd_length){

	// Consider moving these arrays to the heap	
	char cmd[MAX_COMMAND_LENGTH];
	char r_buf[MAX_READ_BUFFER_SIZE + 1];
	size_t b_read;
	size_t i;

	// *start points to the first available element in buffer
	char *start = r_buf;
	// *end points to the end of the command, which is basically
	// the first element of the COMMAND_SEPARATOR
	char *end = r_buf;

	memset(cmd, '\0', MAX_COMMAND_LENGTH);

	// Since we operate only on MAX_READ_BUFFER_SIZE elements
	// the last character is always \0.
	memset(r_buf, '\0', MAX_READ_BUFFER_SIZE + 1);


	iddqd_cmd pcmd;// p stands for "parsed"
	
	while (1){  

		b_read = read(fd, start, (r_buf + MAX_READ_BUFFER_SIZE) - start); // ... * sizeof(char)
		start = start + b_read;
		end = strstr(r_buf, COMMAND_SEPARATOR);
		
		// The first thing in the buffer is COMMAND_SEPARATOR
		// TODO: Shift everything COMMAND_SEPARATOR_LENGTH left and try again
		if(r_buf == end) {
			ALOGI("start == end");
			break;
		}	

		// Either command is too long or the client uses unsupported
		// COMMAND_SEPARATOR
		if(end == NULL && b_read == 0) {
			ALOGI("end == null");
			break;	
		} 

		// Command is too long
		if((end - r_buf) > MAX_COMMAND_LENGTH) {
			ALOGI("Command size exceeded");
			memset(r_buf, '\0', MAX_READ_BUFFER_SIZE);
			start = r_buf;
			end = r_buf;
			continue;
		}

		strncpy(cmd, r_buf, end - r_buf);// ... * sizeof(char)
		end = end + COMMAND_SEPARATOR_LENGTH; 

		// TODO: employ MACRO or function
		for (i = 0; i < (r_buf + MAX_READ_BUFFER_SIZE) - end - 1; i++){
			*(r_buf + i) = *(end + i);
		}
		start = ((r_buf + MAX_READ_BUFFER_SIZE) - end);
		end = r_buf;


		if (!parse_cmd(cmd, &pcmd)){
			ALOGI("Command is %s\n", pcmd.cmd);
			ALOGI("Argument is %s\n", pcmd.arg);
		}
		
		memset(cmd, '\0', MAX_COMMAND_LENGTH);
		
	}
	
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
