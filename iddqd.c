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
// TOKEN is a sequence of alphanumeric characters. COMMAND is a
// sequence of space-separated tokens.
#define MAX_TOKEN_LENGTH 23
#define MAX_TOKEN_SIZE MAX_TOKEN_LENGTH + 1
#define MAX_COMMAND_LENGTH 27
#define MAX_COMMAND_SIZE MAX_COMMAND_LENGTH + 1
#define TOKEN_SEPARATORS " \t\n"
#define COMMAND_SEPARATOR "-"
#define COMMAND_SEPARATOR_LENGTH 1

#define MAX_READ_BUFFER_LENGTH (MAX_COMMAND_LENGTH + MAX_TOKEN_LENGTH) * 2
#define MAX_READ_BUFFER_SIZE MAX_READ_BUFFER_LENGTH + 1

#define MAX_READ_BUFFER_LENGTH (MAX_COMMAND_LENGTH + MAX_TOKEN_LENGTH) * 2
#define MAX_READ_BUFFER_SIZE MAX_READ_BUFFER_LENGTH + 1

#define MAX_DEVICE_INFO_LENGTH 256
#define MAX_DEVICE_INFO_COUNT 4
#define MAX_WRITE_BUFFER_LENGTH MAX_DEVICE_INFO_LENGTH *MAX_DEVICE_INFO_COUNT
#define MAX_WRITE_BUFFER_SIZE MAX_WRITE_BUFFER_LENGTH + 1

#define MESSAGE_NO_SUCH_COMMAND "Unknown command"
#define MESSAGE_NO_SUCH_DEVICE "No such device"

#define INPUT_DEVICE_INFO_FILE "/proc/bus/input/devices"

#include <cutils/log.h>
#include <cutils/sockets.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <unistd.h>

typedef struct {
  char cmd[MAX_TOKEN_SIZE];
  char arg[MAX_TOKEN_SIZE];
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
static inline int get_max(int first, int second) {
  return (first < second) ? (second) : (first);
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

int make_nonblocking(int fd) {
  int current_flags = fcntl(fd, F_GETFL);

  if (current_flags < 0) {
    return 1;
  }
  current_flags |= O_NONBLOCK;

  return fcntl(fd, F_SETFL, current_flags) != 0;
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

int parse_cmd(char *string, iddqd_cmd *res) {

  char *n_token = strtok(string, TOKEN_SEPARATORS);
  size_t string_length;

  string_length = strlen(n_token);
  if (n_token == NULL || (string_length == 0) ||
      (string_length >= MAX_TOKEN_LENGTH)) {
    return 1;
  }

  strcpy(res->cmd, n_token);

  // This would be a loop if we had variable number of argument.
  n_token = strtok(NULL, TOKEN_SEPARATORS);
  string_length = strlen(n_token);

  if (n_token == NULL || (string_length == 0) ||
      (string_length >= MAX_TOKEN_LENGTH)) {
    return 1;
  }

  strcpy(res->arg, n_token);

  return 0;
}

/*
 * Function: read_from_socket
 * ----------------------
 * Read up to N characters from the provided file descriptor into a given
 * buffer.
 *
 * n1: a file descriptor
 * n2: a pointer to a buffer
 * n3: size of the buffer
 *
 * returns:  number of bytes read
 *
 */
ssize_t read_from_socket(int fd, char *rbuf, size_t buffer_length) {

  size_t taken_space;
  size_t free_space;
  ssize_t result;

  taken_space = strlen(rbuf);
  free_space = buffer_length - taken_space;

  if (free_space > 0) {
    // TODO: process return code
    result = read(fd, rbuf + taken_space, free_space);
    if (result >= 0) {
      rbuf[taken_space + result] = '\0';
    } else {
      result = 0;
    }
  } else {
    result = 0;
  }
  ALOGI("read_from_socket exiting, read %d \n", result);

  return result;
}

/*
 * Function: shift_left
 * --------------------
 * Shift contents of the given buffer starting from _offset_ position
 * to the left.
 *
 * n1: a pointer to a buffer
 * n2: buffer length  (i.e. its size -1)
 * n3: offset
 *
 * returns: nothing
 *
 */

void shift_left(char *buf, const size_t length, const size_t offset) {
  // TODO: Rewrite with strcpy
  for (size_t i = 0; i < length - 1 - offset; i++) {
    *(buf + i) = *(buf + offset + i);
    *(buf + i + 1) = '\0';
  }
}

/*
 * Function: parse_input
 * ----------------------
 * Read up to N characters from the provided file descriptor into a given
 * buffer.
 *
 * n1: a file descriptor
 * n2: a pointer to a buffer
 * n3: size of the buffer
 *
 * returns:  length of the parsed command (excluding \0)
 *
 */

size_t parse_input(char *rbuf, char *cmd, size_t length) {

  char *delimiter;
  delimiter = strstr(rbuf, COMMAND_SEPARATOR);
  *cmd = '\0';

  // Either command is too long or the client uses unsupported
  // COMMAND_SEPARATOR
  if (delimiter == NULL) {
    ALOGI("Delimiter not found\n");
    *rbuf = '\0';
    return 0;
  }

  // The first thing in the buffer is COMMAND_SEPARATOR
  // TODO: Shift everything COMMAND_SEPARATOR_LENGTH left and try again
  if (rbuf == delimiter) {
    ALOGI("Delimiter at start\n");
    shift_left(rbuf, MAX_READ_BUFFER_LENGTH, COMMAND_SEPARATOR_LENGTH);
    return 0;
  }

  // Command is too long
  if ((delimiter - rbuf) > MAX_COMMAND_LENGTH) {
    ALOGI("Command size exceeded");
    size_t offset = delimiter - rbuf;
    shift_left(rbuf, MAX_READ_BUFFER_LENGTH, offset);
    return 0;
  }

  strncpy(cmd, rbuf, delimiter - rbuf);
  cmd[delimiter - rbuf] = '\0'; /* This is added to guarantee that
  the last symbol in cmd is \0. You should not care about
  buffer overflow since MAX_COMMAND_LENGTH, against which the source is tested
  a few lines above, is smaller than MAX_COMMAND_SIZE.*/

  delimiter = delimiter + COMMAND_SEPARATOR_LENGTH;
  size_t offset = delimiter - rbuf;
  shift_left(rbuf, MAX_READ_BUFFER_LENGTH, offset);
  return 1;
}

/*
 * Function: get_next_command
 * ----------------------
 * Retrieve a command from a given file descriptor. Command is a character
 * sequence
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

int get_next_command(int fd, char *rbuf, char *cmd) {
  size_t bytes_read_cnt;
  size_t parsed_cmd_length;

  do {
    bytes_read_cnt = read_from_socket(fd, rbuf, (size_t)MAX_READ_BUFFER_LENGTH);
    parsed_cmd_length = parse_input(rbuf, cmd, (size_t)MAX_COMMAND_LENGTH);
  } while (bytes_read_cnt > 0 && parsed_cmd_length == 0);

  return parsed_cmd_length;
}

/* Function: execute_get_input_device_info
 * ---------------------------------------
 * Copies device description taken from /proc/bus/input/devices
 * to a given buffer. Device is identified by its position in the file.
 * If device is not found, writes an error message to the buffer.
 *
 * n1: a pointer to an output char buffer
 * n2: buffer length (i.e. without trailing \0)
 * n3: a pointer to a iddqd_cmd struct
 *
 * returns: nothing
 */

void execute_get_input_device_info(char *wbuf, size_t wbuf_length,
                                   iddqd_cmd *pcmd) {
  // Parsing and checking argument
  char *end;
  errno = 0;
  long int argument = strtol(pcmd->arg, &end, 10I);

  if (errno != 0 || pcmd->arg == end) {
    // We really don't care what exact error it was.
    ALOGE("Get input device info. Invalid argument %s\n", pcmd->arg);
    return;
  }
  if (argument > INT_MAX || argument < INT_MIN) {
    ALOGE("Get input device info. The argument %s is too large.\n", pcmd->arg);
    return;
  }

  char device_found = 0;
  int device_no = (int)argument;
  int current_device_no = -1;
  char *line = NULL;
  size_t length = 0;
  ssize_t read;
  FILE *device_list_stream;

  device_list_stream = fopen(INPUT_DEVICE_INFO_FILE, "r");
  if (device_list_stream == NULL) {
    return;
  }

  while ((read = getline(&line, &length, device_list_stream)) != -1) {
    if (strncmp(line, "I: ", 2) == 0) {
      current_device_no++;
    }
    if (current_device_no == device_no) {
      device_found = 1;
      if (wbuf_length - strlen(wbuf) >= (size_t)read) {
        strcat(wbuf, line);
      }
    }
  }

  if (!device_found) {
    execute_no_such_device(wbuf, MAX_WRITE_BUFFER_LENGTH, pcmd);
  }

  free(line);
  fclose(device_list_stream);
}

/* Function: execute_no_such_command
 * ---------------------------------
 * Writes an error message to the provided character buffer.
 *
 * n1: pointer to a char buffer
 * n2: buffer length
 * n3: pointer to a iddqd_cmd structure.
 *
 * returns: nothing
 */

void execute_no_such_command(char *wbuf, size_t wbuf_length, iddqd_cmd *pcmd) {
  size_t space_left = wbuf_length - strlen(wbuf);
  size_t response_length =
      strlen(MESSAGE_NO_SUCH_COMMAND) + strlen(pcmd->cmd) + 1;

  if (response_length <= space_left) {
    snprintf(wbuf + strlen(wbuf), space_left, "%s: %s\n",
             MESSAGE_NO_SUCH_COMMAND, pcmd->cmd);
  } else {
    // TODO clear buffer? do nothing?
  }
}

/* Function: execute_no_such_device
 * ---------------------------------
 * Writes an error message to the provided character buffer.
 *
 * n1: pointer to a char buffer
 * n2: buffer length
 * n3: pointer to a iddqd_cmd structure.
 *
 * returns: nothing
 */

void execute_no_such_device(char *wbuf, size_t wbuf_length, iddqd_cmd *pcmd) {
  size_t space_left = wbuf_length - strlen(wbuf);
  size_t response_length =
      strlen(MESSAGE_NO_SUCH_COMMAND) + strlen(pcmd->arg) + 1;

  if (response_length <= space_left) {
    snprintf(wbuf + strlen(wbuf), space_left, "%s: %s\n",
             MESSAGE_NO_SUCH_DEVICE, pcmd->arg);
  } else {
    // TODO clear buffer? do nothing?
  }
}

/* Function: execute_command
 * -------------------------
 * Executes a command - i.e. performs command-dependent argument check,
 * takes necessary actions and returns a result.
 *
 * n1: a pointer to an output char buffer
 * n2: buffer length (i.e. without trailing \0)
 * n3: a pointer to a iddqd_cmd struct
 *
 * returns: TODO
 *
 */

void execute_command(char *wbuf, size_t wbuf_length, iddqd_cmd *pcmd) {
  if (strcmp(pcmd->cmd, "get_input_device_info") == 0) {
    execute_get_input_device_info(wbuf, MAX_WRITE_BUFFER_LENGTH, pcmd);
  } else {
    // Command is not supported
    execute_no_such_command(wbuf, MAX_WRITE_BUFFER_LENGTH, pcmd);
  }
}

/*
 * Function: process_cmds
 * ----------------------
 * Reads commands from file. Commands are separated by COMMAND_SEPARATOR.
 *
 * n1: a file descriptor
 * n2: max command length in characters
 * n3: pointer to an output buffer
 * n4: buffer length (i.e. without trailing \0)
 *
 * returns:  0 on succes, 1 on error
 *
 */

static int process_cmds(int fd, int max_cmd_length, char *wbuf,
                        size_t wbuf_length) {

  char cmd[MAX_COMMAND_SIZE];
  char r_buf[MAX_READ_BUFFER_SIZE];
  cmd[0] = '\0';
  r_buf[0] = '\0';
  iddqd_cmd pcmd; // p stands for "parsed"

  size_t b_read;
  size_t i;

  while (get_next_command(fd, r_buf, cmd) > 0) {
    if (!parse_cmd(cmd, &pcmd)) {
      ALOGI("Command is %s\n", pcmd.cmd);
      ALOGI("Argument is %s\n", pcmd.arg);
      execute_command(wbuf, MAX_WRITE_BUFFER_LENGTH, &pcmd);
    }
  }

  return 0;
}

/*
 * Function: write_response
 * ------------------------
 * Writes contents of a given char buffer to a fd. After everything is written
 * clears the buffer.
 * n1: file descriptor
 * n2: pointer to a char buffer
 *
 * returns: nothing
 *
 */

void write_response(int fd, char *wbuf) {
  ssize_t bytes_written_cnt;
  bytes_written_cnt = write(fd, wbuf, strlen(wbuf));
  if (bytes_written_cnt < 0) {
    ALOGE("Unable to write to a socket (%s)\n", strerror(errno));
  } else {
    *wbuf = '\0';
  }
  // Should I use feature test here?
  fsync(fd);
}

int main() {

  char *wbuf = malloc(sizeof(char) * MAX_WRITE_BUFFER_SIZE);
  int l_socket_fd = -1;
  int c_socket_fd = -1;
  fd_set read_fds;
  fd_set write_fds;
  *wbuf = '\0';

  l_socket_fd = android_get_control_socket(SOCKET_NAME);

  if (l_socket_fd < 0) {
    ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
    return -1;
  }

  if (make_nonblocking(l_socket_fd)) {
    ALOGE("Unable to modify inputdevinfo_socket flags. (%s)\n",
          strerror(errno));
  }

  if (listen(l_socket_fd, 0) < 0) {
    ALOGE("Unable to open inputdevinfo_socket (%s)\n", strerror(errno));
    return -1;
  }

  while (1) {
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);

    FD_SET(l_socket_fd, &read_fds);

    if (c_socket_fd >= 0) {
      // &&  fcntl(c_socket_fd, F_GETFD) >= 0 ?
      FD_SET(c_socket_fd, &read_fds);
    }

    if (c_socket_fd >= 0 && strlen(wbuf) > 0) {
      FD_SET(c_socket_fd, &write_fds);
    }

    int retval = select(get_max(c_socket_fd, l_socket_fd) + 1, &read_fds,
                        &write_fds, NULL, NULL);

    if (retval <= 0) {
      ALOGI("Error\n");
      // Should I check for EBADR?
      break;
    }

    if (FD_ISSET(l_socket_fd, &read_fds)) {
      ALOGI("Connection attempt\n");
      if (c_socket_fd < 0) {
        c_socket_fd = accept(l_socket_fd, NULL, NULL);
      }

      if (make_nonblocking(c_socket_fd)) {
        ALOGE("Unable to modify socket flags. (%s)\n", strerror(errno));
      }
    }

    if (c_socket_fd >= 0 && FD_ISSET(c_socket_fd, &write_fds)) {
      write_response(c_socket_fd, wbuf);
    }

    if (c_socket_fd >= 0 && FD_ISSET(c_socket_fd, &read_fds)) {

      int unread_bytes_count;
      if (ioctl(c_socket_fd, FIONREAD, &unread_bytes_count)) {
        ALOGE(
            "Attempt to check if client socket is closed resulted in error.\n");
      } else if (unread_bytes_count == 0) {
        // TODO: Check return code?
        close(c_socket_fd);
        c_socket_fd = -1;
      } else {
        process_cmds(c_socket_fd, MAX_COMMAND_LENGTH, wbuf,
                     MAX_WRITE_BUFFER_LENGTH);
      }
    }
  }

  free(wbuf);
  return 0;
}
