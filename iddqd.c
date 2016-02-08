/*
 * TODO: Insert a copyright notice.
 *
 * iddqd - Input Device Description Query Daemon
 *
 */

#define LOG_TAG "iddqd"
#define SOCKET_NAME "inputdevinfo_socket"
#define MAX_COMMAND_LENGTH 27
#define COMM_FILE_OPEN_MODE "a+"

#include <cutils/sockets.h>
#include <cutils/log.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>


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
