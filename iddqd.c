/*
 * TODO: Insert a copyright notice.
 *
 * iddqd - Input Device Description Query Daemon
 *
 */

#define LOG_TAG "iddqd"
#define SOCKET_NAME "inputdevinfo_socket"

// NB that WHATEVER_SIZE is basically WHATEVER_LENGTH plus one
// to take into account string termination characher.
#define MAX_TOKEN_LENGTH 23
#define MAX_TOKEN_SIZE MAX_TOKEN_LENGTH + 1
#define MAX_COMMAND_LENGTH 27
#define MAX_COMMAND_SIZE MAX_COMMAND_LENGTH + 1
#define TOKEN_SEPARATORS  " \t\n"
#define COMMAND_SEPARATOR "-"
#define COMMAND_SEPARATOR_LENGTH 1

#define MAX_READ_BUFFER_LENGTH (MAX_COMMAND_LENGTH + MAX_TOKEN_LENGTH) * 2
#define MAX_READ_BUFFER_SIZE MAX_READ_BUFFER_LENGTH + 1

#define MAX_READ_BUFFER_LENGTH (MAX_COMMAND_LENGTH + MAX_TOKEN_LENGTH) * 2
#define MAX_READ_BUFFER_SIZE MAX_READ_BUFFER_LENGTH + 1

#include <cutils/sockets.h>
#include <cutils/log.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <string.h>

typedef struct{
	char cmd[MAX_TOKEN_SIZE];
	char arg[MAX_TOKEN_SIZE];
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
	ALOGI("Entered parse_cmd\n");

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
 * Function: read_from_socket
 * ----------------------
 * Read up to N characters from the provided file descriptor into a given buffer.
 *
 * n1: a file descriptor 
 * n2: a pointer to a buffer
 * n3: size of the buffer
 *
 * returns:  number of bytes read
 *
 */
size_t read_from_socket(int fd, char *rbuf, size_t buffer_size){
	ALOGI("Entered read_from_socket\n");

	ALOGI("Entered read_from_socket\n");

	size_t taken_space;
	size_t free_space;
	size_t result;
	
	taken_space = strlen(rbuf);
	free_space = buffer_size - taken_space;
	
	if(free_space > 0){ 
		//TODO: process return code
		result = read(fd, rbuf + taken_space, free_space);
	} else {
		result = 0;
	}
	ALOGI("read_from_socket exiting, read %d \n", result);	
	ALOGI("buffer is %s\n", rbuf);	

	return result;
}

/*
 * Function: parse_input
 * ----------------------
 * Read up to N characters from the provided file descriptor into a given buffer.
 *
 * n1: a file descriptor 
 * n2: a pointer to a buffer
 * n3: size of the buffer
 *
 * returns:  length of the parsed command (excluding \0)
 *
 */

size_t parse_input(char *rbuf, char *cmd, size_t length){

	ALOGI("Entered parse_input\n");	
	ALOGI("buffer is %s\n", rbuf);	
	char *delimiter;
	delimiter = strstr(rbuf, COMMAND_SEPARATOR);
	*cmd='\0';

	// Either command is too long or the client uses unsupported
	// COMMAND_SEPARATOR
	if (delimiter == NULL){
		ALOGI("Delimiter not found\n");
		*rbuf = '\0';
		return 0;
	}

	// The first thing in the buffer is COMMAND_SEPARATOR
	// TODO: Shift everything COMMAND_SEPARATOR_LENGTH left and try again
	if(rbuf == delimiter) {
		ALOGI("Delimiter at start\n");
		for (size_t i = 0; i < MAX_READ_BUFFER_LENGTH - 1 - COMMAND_SEPARATOR_LENGTH; i++){
			*(rbuf + i) = *(rbuf + COMMAND_SEPARATOR_LENGTH + i);
			*(rbuf + i + 1) = '\0';
		}
		return 0;
	}

	// Command is too long
	if((delimiter - rbuf) > MAX_COMMAND_LENGTH) {
		ALOGI("Command size exceeded");
		size_t offset = delimiter - rbuf;
		for (size_t i = 0; i < MAX_READ_BUFFER_LENGTH - 1 - offset; i++){
			*(rbuf + i) = *(rbuf + offset + i);
			*(rbuf + i + 1) = '\0';
		}

		return 0;
	}

	strncpy(cmd, rbuf, delimiter - rbuf);// ... * sizeof(char)
	delimiter = delimiter + COMMAND_SEPARATOR_LENGTH; 
	size_t offset = delimiter - rbuf;

	// TODO: employ MACRO or function
	for (size_t i = 0; i < MAX_READ_BUFFER_LENGTH - 1 - offset ; i++){
		*(rbuf + i) = *(delimiter + i);
		*(rbuf + i + 1) = '\0';
	}

	ALOGI("Exitiong parse_input\n");	
	ALOGI("buffer is %s\n", rbuf);	
	ALOGI("cmd is %s\n", cmd);	
	return 1;
}

/*
 * Function: get_next_command
 * ----------------------
 * Retrieve a command from a given file descriptor. Command is a character sequence
 * terminated by COMMAND_SEPARATOR.
 * First it reads a few bytes from the fd to the buffer, then parses contents of
 * that buffer.
 *
 * n1: a file descriptor 
 * n2: a pointer to read buffer
 * n3: a pointer to write buffer
 *
 * returns:  number of characters written to the write (output) buffer.
 *
 */

int get_next_command(int fd, char *rbuf, char *cmd){
	ALOGI("Entered get_next_command\n");

	size_t bytes_read_cnt;
	size_t parsed_cmd_length;
	size_t result;

	while (1){
		bytes_read_cnt = read_from_socket(fd, rbuf, (size_t) MAX_READ_BUFFER_LENGTH);
		parsed_cmd_length = parse_input(rbuf, cmd, (size_t) MAX_COMMAND_LENGTH);

		if (parsed_cmd_length > 0 ){
			result = parsed_cmd_length;
			break;
		}else if (bytes_read_cnt == 0){
			result = 0;
			break;
		}
	}
	ALOGI("get_next_command exiting\n");	
	return result;
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
	ALOGI("Entered process_cmds\n");
	
	char *cmd = malloc(sizeof(char) * MAX_COMMAND_SIZE);
	char *r_buf = malloc(sizeof(char) * MAX_READ_BUFFER_SIZE);
	*cmd = '\0';
	*r_buf = '\0';
	iddqd_cmd pcmd;// p stands for "parsed"

	size_t b_read;
	size_t i;

	while(get_next_command(fd, r_buf, cmd) > 0) {
		ALOGI("get_next_command\n");

		if (!parse_cmd(cmd, &pcmd)){
			ALOGI("Command is %s\n", pcmd.cmd);
			ALOGI("Argument is %s\n", pcmd.arg);
		}
	}
	
	free(cmd);
	free(r_buf);
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
