/*
 * TODO: Insert a copyright notice.
 *
 * iddqd - Input Device Description Query Daemon
 *
 */

#define LOG_TAG "iddqd"
#define SOCKET_NAME "inputdevinfo_socket"

#include <cutils/sockets.h>
#include <cutils/log.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>

int main() {

	int socket_fd;
	fd_set read_fds;

	socket_fd = android_get_control_socket(SOCKET_NAME);
	
	if (socket_fd < 0 ){
		ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
		return -1;
	}

	while(1) {
		FD_ZERO(&read_fds);
		FD_SET(socket_fd, &read_fds);

		int retval = select(socket_fd + 1, &read_fds, NULL, NULL, NULL);

		if(retval < 0){
			ALOGI("Error reading from socket %s\n", strerror(errno));
		} else if (retval > 0 && FD_ISSET(socket_fd, &read_fds)) {
			ALOGI("We have some data to read from our only socket\n");
		} else {
			;
		}
		
	}

return 0;

}
