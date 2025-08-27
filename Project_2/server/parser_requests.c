
#include "common/io.h"
#include "common/constants.h"
#include <stdio.h>
#include <string.h>

int parse_setup(int rx, char *req_pipe_path, char *resp_pipe_path) { //FIXME -> transformar isto em apenas 1 read em vez de 3

  char op_code;

  int return_value = parse_str_pipe(rx, &op_code, 1);
  if (return_value == 1) {return 1;}
  if (return_value == SIGNAL_DETECTED) {return SIGNAL_DETECTED;}
  if (return_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  return_value = parse_str_pipe(rx, req_pipe_path, MAX_FIFO_PATHNAME);
  if (return_value == 1) {return 1;}
  if (return_value == SIGNAL_DETECTED) {return SIGNAL_DETECTED;}
  if (return_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  return_value = parse_str_pipe(rx, resp_pipe_path, MAX_FIFO_PATHNAME);
  if (return_value == 1) {return 1;}
  if (return_value == SIGNAL_DETECTED) {return SIGNAL_DETECTED;}
  if (return_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  return 0;
}

int parse_create(int req_pipe, unsigned int *event_id, size_t *num_rows, size_t *num_cols) {

  int parse_value = parse_uns_int_pipe(req_pipe, event_id);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  parse_value = parse_size_t_pipe(req_pipe, num_rows);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  parse_value = parse_size_t_pipe(req_pipe, num_cols);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  return 0;
}


int parse_reserve(int req_pipe, unsigned int *event_id, 
                  size_t *num_seats, size_t *xs, size_t *ys) {

  int parse_value = parse_uns_int_pipe(req_pipe, event_id);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}
  
  parse_value = parse_size_t_pipe(req_pipe, num_seats);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  parse_value = parse_size_t_array_pipe(req_pipe, xs, *num_seats);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  parse_value = parse_size_t_array_pipe(req_pipe, ys, *num_seats);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  return 0;
}

int parse_show(int req_pipe, unsigned int *event_id) {

  int parse_value = parse_uns_int_pipe(req_pipe, event_id);
  if (parse_value == 1) {return 1;}
  if (parse_value == PIPE_CLOSED) {return PIPE_CLOSED;}

  return 0;
}
