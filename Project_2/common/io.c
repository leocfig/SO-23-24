#include "io.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h>


#include "common/constants.h"



int parse_uint(int fd, unsigned int *value, char *next) {
  char buf[16];

  int i = 0;
  while (1) {
    ssize_t read_bytes = read(fd, buf + i, 1);

    if (read_bytes == -1) {
      return 1;
    } else if (read_bytes == 0) {
      *next = '\0';
      break;
    }
    
    *next = buf[i];

    if (buf[i] > '9' || buf[i] < '0') {
      buf[i] = '\0';
      break;
    }

    i++;
  }

  unsigned long ul = strtoul(buf, NULL, 10);

  if (ul > UINT_MAX) {
    return 1;
  }

  *value = (unsigned int)ul;

  return 0;
}


int parse_int_pipe(int pipe, int *value) {
  ssize_t bytes_read = read(pipe, value, sizeof(int));

  if (bytes_read == -1) {
    fprintf(stderr, "read error: %s\n", strerror(errno));
    return 1;
  }

  if (bytes_read == 0) {
    return PIPE_CLOSED;
  }

  return 0;
}


int parse_uns_int_pipe(int pipe, unsigned int *value) {
  ssize_t bytes_read = read(pipe, value, sizeof(unsigned int));

  if (bytes_read == -1) {
    fprintf(stderr, "read error: %s\n", strerror(errno));
    return 1;
  }

  if (bytes_read == 0) {
    return PIPE_CLOSED;
  }

  return 0;
}


int parse_size_t_pipe(int pipe, size_t *value) {
  ssize_t bytes_read = read(pipe, value, sizeof(size_t));

  if (bytes_read == -1) {
    fprintf(stderr, "read error: %s\n", strerror(errno));
    return 1;
  }

  if (bytes_read == 0) {
    return PIPE_CLOSED;
  }

  return 0;
}


int parse_size_t_array_pipe(int pipe, size_t *values, size_t num_elements) {
  size_t bytes_to_read = num_elements * sizeof(size_t);
  size_t bytes_read = 0;

  while (bytes_read < bytes_to_read) {
    ssize_t result = read(pipe, (char*)values + bytes_read, bytes_to_read - bytes_read);

    if (result == -1) {
      fprintf(stderr, "read error: %s\n", strerror(errno));
      return 1;
    }

    if (result == 0) {
      return PIPE_CLOSED;
    }

    bytes_read += (size_t)result;
  }

  return 0;
}

int parse_uns_int_array_pipe(int pipe, unsigned int *values, size_t num_elements) {
  ssize_t bytes_read = read(pipe, values, num_elements * sizeof(unsigned int));

  if (bytes_read == -1) {
    fprintf(stderr, "read error: %s\n", strerror(errno));
    return 1;
  }

  if (bytes_read == 0) {
    return PIPE_CLOSED;
  }

  // Check if the correct number of bytes were read
  if ((size_t)bytes_read != num_elements * sizeof(unsigned int)) {
    fprintf(stderr, "incomplete read from pipe\n");
    return 1;
  }

  return 0;
}



int print_uint(int fd, unsigned int value) {
  char buffer[16];
  size_t i = 16;

  for (; value > 0; value /= 10) {
    buffer[--i] = '0' + (char)(value % 10);
  }

  if (i == 16) {
    buffer[--i] = '0';
  }

  while (i < 16) {
    ssize_t written = write(fd, buffer + i, 16 - i);
    if (written == -1) {
      return 1;
    }

    i += (size_t)written;
  }

  return 0;
}


int print_int_pipe(int pipe, const int value) {

  errno = 0;
  ssize_t written = write(pipe, &value, sizeof(int));

  if (errno == EPIPE) {
    // The write was interrupted by the sigpipe signal, meaning that the pipe was closed
    return PIPE_CLOSED;
  }

  if (written == -1) {
    fprintf(stderr, "write error: %s\n", strerror(errno));
    return 1;
  }

  if (written != sizeof(unsigned int)) {
    fprintf(stderr, "incomplete write to pipe\n");
    return 1;
  }

  return 0;
}


int print_uns_int_pipe(int pipe, const unsigned int value) {

  errno = 0;
  ssize_t written = write(pipe, &value, sizeof(unsigned int));

  if (errno == EPIPE) {
    // The write was interrupted by the sigpipe signal, meaning the pipe was closed
    return PIPE_CLOSED;
  }

  if (written == -1) {
    fprintf(stderr, "write error: %s\n", strerror(errno));
    return 1;
  }

  if (written != sizeof(unsigned int)) {
    fprintf(stderr, "incomplete write to pipe\n");
    return 1;
  }

  return 0;
}


int print_size_t_pipe(int pipe, const size_t value) {

  errno = 0;
  ssize_t written = write(pipe, &value, sizeof(size_t));

  if (errno == EPIPE) {
    // The write was interrupted by the sigpipe signal, meaning the pipe was closed
    return PIPE_CLOSED;
  }

  if (written == -1) {
    fprintf(stderr, "write error: %s\n", strerror(errno));
    return 1;
  }

  if (written != sizeof(size_t)) {
    fprintf(stderr, "incomplete write to pipe\n");
    return 1;
  }

  return 0;
}

int print_size_t_array_pipe(int pipe, const size_t *values, size_t num_elements) {

  errno = 0;
  ssize_t written = write(pipe, values, num_elements * sizeof(size_t));

  if (errno == EPIPE) {
    // The write was interrupted by the sigpipe signal, meaning the pipe was closed
    return PIPE_CLOSED;
  }

  if (written == -1) {
    fprintf(stderr, "write error: %s\n", strerror(errno));
    return 1;
  }

  if ((size_t)written != num_elements * sizeof(size_t)) {
    fprintf(stderr, "incomplete write to pipe\n");
    return 1;
  }

  return 0;
}

int print_uns_int_array_pipe(int pipe, const unsigned int *values, size_t num_elements) {

  errno = 0;
  ssize_t written = write(pipe, values, num_elements * sizeof(unsigned int));

  if (errno == EPIPE) {
    // The write was interrupted by the sigpipe signal, meaning the pipe was closed
    return PIPE_CLOSED;
  }

  if (written == -1) {
    fprintf(stderr, "write error: %s\n", strerror(errno));
    return 1;
  }

  if ((size_t)written != num_elements * sizeof(unsigned int)) {
    fprintf(stderr, "incomplete write to pipe\n");
    return 1;
  }

  return 0;
}



int parse_str_pipe(int pipe, char *str, unsigned int size) {

  unsigned int done = 0;
  while (done < size) {

    errno = 0;  // Resets errno
    ssize_t bytes_read = read(pipe, str + done, size - done);

    if (bytes_read == -1) {
      if (errno == EINTR) {
        if (done == 0) {
          // The read was interrupted by a signal
          printf("Entrou no signal\n");
          return SIGNAL_DETECTED;
        }
        else {
          continue;
        }
      }
      fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
      return 1;
    }

    if (bytes_read == 0)
      return PIPE_CLOSED;

    done += (unsigned int)bytes_read;
  }
  return 0;
}

int print_str(int fd, const char *str) {
  size_t len = strlen(str);
  while (len > 0) {
    ssize_t written = write(fd, str, len);
    
    if (written == -1) {
      return 1;
    }

    str += (size_t)written;
    len -= (size_t)written;
  }

  return 0;
}


int print_str_pipe(int pipe, const char *str, int size) {

  errno = 0;

  size_t len = (size_t)size;
  while (len > 0) {
    ssize_t written = write(pipe, str, len);

    if (errno == EPIPE) {
      // The write was interrupted by the sigpipe signal, meaning the pipe was closed
      return PIPE_CLOSED;
    }

    if (written == -1) {
      return 1;
    }

    str += (size_t)written;
    len -= (size_t)written;
  }

  return 0;
}


int print_output_show(int out_fd, size_t num_rows, size_t num_cols, unsigned int *seats) {

  unsigned int *row = (unsigned int*)malloc(num_cols * sizeof(unsigned int));

  char *char_buffer = (char *)malloc((num_rows * num_cols * (UNS_INT_SIZE + 1) + 1 )* sizeof(char));
  if (char_buffer == NULL) {
    fprintf(stderr, "Memory allocation for char_buffer failed\n");
    return 1;
  }

  char_buffer[0]=0;

  for (size_t i = 0; i <num_rows; i++) {
    for (size_t j = 0, k = 0; j < num_cols; j++) {
      row[k++] = seats[i*num_cols + j];
    }
    char *buffer = buffer_to_string(row, num_cols, SHOW_KEY);
    strcat(char_buffer,buffer);
    free(buffer);
  }
  if (print_str(out_fd, char_buffer)) { return 1; }
  free(row);
  free(char_buffer);
  return 0;
}


char *buffer_to_string(const unsigned int *buffer, size_t buffer_size, int function_called) {
  size_t size = (buffer_size * (UNS_INT_SIZE + 1) + 1);
  char *char_buffer = (char *)malloc(size * sizeof(char));

  if (char_buffer == NULL) {
    fprintf(stderr, "Memory allocation for buffer failed\n");
    return NULL;
  }

  size_t len = (size_t)snprintf(char_buffer, size, "%u", buffer[0]);

  if (len >= size) {
    fprintf(stderr, "Error in snprintf\n");
    free(char_buffer);
    return NULL;
  }

  if (function_called == SHOW_KEY) {
    for (size_t i = 1; i < buffer_size; i++) {
      len += (size_t)snprintf(char_buffer + len, size - len, " %u", buffer[i]);

      if (len >= size) {
        fprintf(stderr, "Error in snprintf\n");
        free(char_buffer);
        return NULL;
      }
    }
  }

  snprintf(char_buffer + len, size - len, "\n");
  return char_buffer;
}