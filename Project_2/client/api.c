#include "api.h"
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#include "common/io.h"
#include "common/constants.h"


unsigned int active_session;

char req_path[MAX_FIFO_PATHNAME];
char resp_path[MAX_FIFO_PATHNAME];
int req_pipe;
int resp_pipe;




int ems_setup(char *req_pipe_path, char *resp_pipe_path, char const* server_pipe_path) {

  strcpy(req_path, req_pipe_path);
  strcpy(resp_path, resp_pipe_path);

  // Removes req_pipe_path if it exists
  if (unlink(req_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_path,
            strerror(errno));
    return 1;
  }

  // Removes resp_pipe_path if it exists
  if (unlink(resp_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_path,
            strerror(errno));
    return 1;
  }

  // Creates req_pipe_path pipe
  if (mkfifo(req_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return 1;
  }

  // Creates resp_pipe_path pipe
  if (mkfifo(resp_path, 0640) != 0) {
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    return 1;
  }

  // Opens pipe for writing the two named pipes
  int tx = open(server_pipe_path, O_WRONLY);
  if (tx == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return 1;
  }

  char setup_buffer[MAX_FIFO_PATHNAME * 2 + 1];

  // Makes the request
  char op = SETUP;
  addNullCharacters(req_path, MAX_FIFO_PATHNAME);
  addNullCharacters(resp_path, MAX_FIFO_PATHNAME);
  memcpy(setup_buffer, &op, 1);
  memcpy(setup_buffer + 1, req_path, MAX_FIFO_PATHNAME);
  memcpy(setup_buffer + MAX_FIFO_PATHNAME + 1, resp_path, MAX_FIFO_PATHNAME);

  // Writes the whole concatenated string in the pipe
  if (print_str_pipe(tx, setup_buffer, MAX_FIFO_PATHNAME * 2 + 1)) { return 1; }

  // Opens req_pipe_path
  req_pipe = open(req_path, O_WRONLY);
  if (req_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return 1;
  }

  // Opens resp_pipe_path
  resp_pipe = open(resp_path, O_RDONLY);
  if (resp_pipe == -1) {
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    return 1;
  }

  // Receives the session ID from the server
  if (parse_uns_int_pipe(resp_pipe, &active_session)) { return 1; }

  return 0;
}


int ems_quit(void) {

  // Makes the request
  char op = QUIT;
  if (print_str_pipe(req_pipe, &op, 1)) { return 1; }
  if (print_uns_int_pipe(req_pipe, active_session)) { return 1; }

  close(req_pipe);
  close(resp_pipe);

  // Removes req_pipe_path
  if (unlink(req_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", req_path,
            strerror(errno));
    return 1;
  }

  // Removes resp_pipe_path
  if (unlink(resp_path) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", resp_path,
            strerror(errno));
    return 1;
  }

  return 0;
}


int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {

  // Makes the request
  char op = CREATE;
  if (print_str_pipe(req_pipe, &op, 1)) { return 1; }
  if (print_uns_int_pipe(req_pipe, active_session)) { return 1; }
  if (print_uns_int_pipe(req_pipe, event_id)) { return 1; }
  if (print_size_t_pipe(req_pipe, num_rows)) { return 1; }    //Juntar os dois?
  if (print_size_t_pipe(req_pipe, num_cols)) { return 1; }

  // Waits for response
  int return_value;
  if (parse_int_pipe(resp_pipe, &return_value)) { return 1; }

  return return_value;
}


int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  
  // Makes the request
  char op = RESERVE;
  if (print_str_pipe(req_pipe, &op, 1)) { return 1; }
  if (print_uns_int_pipe(req_pipe, active_session)) { return 1; }

  // Event id and the number of seats to be reserved
  if (print_uns_int_pipe(req_pipe, event_id)) { return 1; }
  if (print_size_t_pipe(req_pipe, num_seats)) { return 1; }

  if (print_size_t_array_pipe(req_pipe, xs, num_seats)) { return 1; }
  if (print_size_t_array_pipe(req_pipe, ys, num_seats)) { return 1; }

  // Waits for response
  int returned_value; 
  if (parse_int_pipe(resp_pipe, &returned_value)) { return 1; }

  return returned_value;
}


int ems_show(int out_fd, unsigned int event_id) {

  // Makes the request
  char op = SHOW;
  if (print_str_pipe(req_pipe, &op, 1)) { return 1; }
  if (print_uns_int_pipe(req_pipe, active_session)) { return 1; }
  if (print_uns_int_pipe(req_pipe, event_id)) { return 1; }

  // Waits for response
  int returned_value; 
  size_t num_rows, num_cols;
  if (parse_int_pipe(resp_pipe, &returned_value)) { return 1; }

  if (!returned_value){ // Writes the event to the output file if the returned value is 0
    if (parse_size_t_pipe(resp_pipe, &num_rows)) { return 1; }
    if (parse_size_t_pipe(resp_pipe, &num_cols)) { return 1; }

    unsigned int seats[num_cols*num_rows];
    if (parse_uns_int_array_pipe(resp_pipe, seats, num_cols * num_rows)) { return 1; }
    if (print_output_show(out_fd, num_rows, num_cols, seats)) { return 1; }
  }
  return 0;
}

int ems_list_events(int out_fd) {

  char op = LIST_EVENTS;
  if (print_str_pipe(req_pipe, &op, 1)) { return 1; }
  if (print_uns_int_pipe(req_pipe, active_session)) { return 1; }
  
  int returned_value; 
  size_t num_events;
  if (parse_int_pipe(resp_pipe, &returned_value)) { return 1; }

  if (returned_value == 0){ // Lists the events sent from the server to the output file
    if (parse_size_t_pipe(resp_pipe, &num_events)) { return 1; }

    if (num_events == 0){
      if (print_str(out_fd, "No events\n")) { return 1; }
      return 0;
    }

    unsigned int list_event_ids[num_events];
    if (parse_uns_int_array_pipe(resp_pipe, list_event_ids, num_events)) { return 1; }

    if (print_output_list(out_fd, num_events,list_event_ids)) { return 1; }
    return 0;
  } 
  return 1;
}



void addNullCharacters(char *str, size_t targetLength) {
  size_t currentLength = strlen(str);
  size_t nullCharactersNeeded = (currentLength < targetLength) ? (targetLength - currentLength) : 0;

  // Add null characters at the end
  memset(str + currentLength, '\0', nullCharactersNeeded);
}


int print_output_list(int out_fd, size_t num_events, unsigned int *events) {

  char* buffer_list = (char*)malloc((num_events*(8+UNS_INT_SIZE)+1)*sizeof(char));
  if (buffer_list == NULL) {
    fprintf(stderr, "Failed to allocate memory for buffer_list\n");
    return 1;
  }

  buffer_list[0]=0;

  for (size_t k = 0; k < num_events; k++){
    strcat(buffer_list, "Event: ");
    char *buffer = buffer_to_string(&events[k], 1, LIST_KEY);
    strcat(buffer_list,buffer);
    free(buffer);
  }

  if (print_str(out_fd, buffer_list)) { return 1; }
  free(buffer_list);

  return 0;
}

