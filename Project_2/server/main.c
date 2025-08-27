#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

#include "common/constants.h"
#include "common/io.h"
#include "operations.h"
#include "parser_requests.h"
#include "queue_operations.h"





// Variable that indicates whether the SIGUSR1 signal has been received.
volatile sig_atomic_t sigusr1_received = 0;



static void sig_handler() {

  // Sets the signal handler back to our function after each trap.
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    exit(EXIT_FAILURE);
  }
  sigusr1_received = 1;
  return; // Resume execution at point of interruption
}


int process_Op_Codes(Session *session) {

  int active_session = 1;

  while (active_session) {
    
    unsigned int event_id;
    size_t num_rows, num_cols;
    size_t num_seats, num_events;
    size_t xs[MAX_RESERVATION_SIZE];
    size_t ys[MAX_RESERVATION_SIZE];

    unsigned int *event_seats = NULL;
    unsigned int *event_ids = NULL;

    int session_id;
    char op_code;

    int req_pipe = session->req_pipe;
    int resp_pipe = session->resp_pipe;

    // Receives the op_code
    int parse_value = parse_str_pipe(req_pipe, &op_code, 1);
    if (parse_value == 1) {return 1;}
    if (parse_value == PIPE_CLOSED) {
      ems_quit(session);
      break;
    }
    
    // Receives the session_id
    parse_value = parse_int_pipe(req_pipe, &session_id);
    if (parse_value == 1) {return 1;}
    if (parse_value == PIPE_CLOSED) {
      ems_quit(session);
      break;
    }

    int return_value;
    int print_value;

    switch (op_code) {
      case QUIT:
        if (ems_quit(session) != 0) {return 1;}
        active_session = 0;
        break;
      case CREATE:
        parse_value = parse_create(req_pipe, &event_id, &num_rows, &num_cols);
        if (parse_value == 1) {return 1;}
        if (parse_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }
        return_value = ems_create(event_id, num_rows, num_cols);
        print_value = print_int_pipe(session->resp_pipe, return_value);
        if (print_value == 1) {return 1;}
        if (print_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }
        break;
      case RESERVE:
        parse_value = parse_reserve(req_pipe, &event_id, &num_seats, xs, ys);
        if (parse_value == 1) {return 1;}
        if (parse_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }
        return_value = ems_reserve(event_id, num_seats, xs, ys);
        print_value = print_int_pipe(resp_pipe, return_value);
        if (print_value == 1) {return 1;}
        if (print_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }
        break;
      case SHOW:
        parse_value = parse_show(req_pipe, &event_id);
        if (parse_value == 1) {return 1;}
        if (parse_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }
        return_value = ems_show(event_id, &event_seats, &num_rows, &num_cols);
        print_value = print_int_pipe(resp_pipe, return_value);
        if (print_value == 1) {return 1;}
        if (print_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }

        if (!return_value) { // Writes the response to the pipe if the event exists
          print_value = print_size_t_pipe(resp_pipe, num_rows);
          if (print_value == 1) {return 1;}
          if (print_value == PIPE_CLOSED) {
            ems_quit(session);
            active_session = 0;
            break;
          }
          print_value = print_size_t_pipe(resp_pipe, num_cols);
          if (print_value == 1) {return 1;}
          if (print_value == PIPE_CLOSED) {
            ems_quit(session);
            active_session = 0;
            break;
          }
          print_value = print_uns_int_array_pipe(resp_pipe, event_seats, num_rows * num_cols);
          if (print_value == 1) {return 1;}
          if (print_value == PIPE_CLOSED) {
            ems_quit(session);
            active_session = 0;
            break;
          }
        }
        free(event_seats);
        break;
      case LIST_EVENTS:
        return_value = ems_list_events(&event_ids, &num_events);
        print_value = print_int_pipe(resp_pipe, return_value);
        if (print_value == 1) {return 1;}
        if (print_value == PIPE_CLOSED) {
          ems_quit(session);
          active_session = 0;
          break;
        }
        if (return_value == 0) {
          print_value = print_size_t_pipe(resp_pipe, num_events);
          if (print_value == 1) {return 1;}
          if (print_value == PIPE_CLOSED) {
            ems_quit(session);
            active_session = 0;
            break;
          }
          if (num_events) {
            print_value = print_uns_int_array_pipe(resp_pipe, event_ids, num_events);
            if (print_value == 1) {return 1;}
            if (print_value == PIPE_CLOSED) {
              ems_quit(session);
              active_session = 0;
              break;
            }
          }
        }
        free(event_ids);
        break;
      default:
        break;
    }
  }
  return 0;
}



void* handle_client(void* arg) {

  sigset_t signal_mask;

  // Initializes signal_mask as an empty set.
  if (sigemptyset(&signal_mask) != 0) {
    print_error("Failed to initialize the signal mask\n");
  }

  // Adds SIGUSR1 to the empty set.
  if (sigaddset(&signal_mask, SIGUSR1) != 0) {
    print_error("Failed to add SIGUSR1 to the signal mask\n");
  }

  // Adds SIGPIPE to the empty set.
  if (sigaddset(&signal_mask, SIGPIPE) != 0) {
    print_error("Failed to add SIGPIPE to the signal mask\n");
  }
  
  // All the new threads will inherit this signal mask
  if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0) {
    print_error("Failed to set the signal mask\n");
    return NULL;
  }

  ThreadData* threadData = (ThreadData*)arg;
  DynamicBuffer *buffer = threadData->buffer;
  int session_id = threadData->session_id;
  int setup_result;

  while (1) {   // When a thread finishes a session, it goes to retrive the next one
    Session *session = retrieveLastSessionRequest(buffer);
    if (session == NULL) {return NULL;}
    session->session_id = session_id;
    setup_result = ems_setup(session);
    if (setup_result == 1) {return NULL;}
    if (setup_result == PIPE_CLOSED) {
      ems_quit(session);
      continue;
    }
    if (process_Op_Codes(session) != 0) {return NULL;}
  }
}


int createThreads(ThreadData *threads, DynamicBuffer *buffer) {

  for (int i = 0; i < MAX_SESSION_COUNT; i++) {

    threads[i].buffer = buffer;
    threads[i].session_id = i;

    if (pthread_create(&threads[i].threadId, NULL, handle_client, (void*)&threads[i]) != 0) {
      pthread_mutex_lock(&mutex_terminal);
      fprintf(stderr, "Failed to create thread %d\n", i);
      pthread_mutex_unlock(&mutex_terminal);
      return 1;
    }
  }
  return 0;
}


int main(int argc, char* argv[]) {

  // Establish same handler for SIGUSR1.
  if (signal(SIGUSR1, sig_handler) == SIG_ERR) {
    return 1;
  }
  
  if (argc < 2 || argc > 3) {
    pthread_mutex_lock(&mutex_terminal);
    fprintf(stderr, "Usage: %s\n <pipe_path> [delay]\n", argv[0]);
    pthread_mutex_unlock(&mutex_terminal);
    return 1;
  }

  char* endptr;
  unsigned int state_access_delay_us = STATE_ACCESS_DELAY_US;
  if (argc == 3) {
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      print_error("Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_us = (unsigned int)delay;
  }

  if (ems_init(state_access_delay_us)) {
    print_error("Failed to initialize EMS\n");
    return 1;
  }

  // Fifo server pathname
  char *register_fifo = argv[1];

  // Removes pipe if it exists
  if (unlink(register_fifo) != 0 && errno != ENOENT) {
    pthread_mutex_lock(&mutex_terminal);
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", register_fifo,
            strerror(errno));
    pthread_mutex_unlock(&mutex_terminal);
    return 1;
  }

  // Creates the server's pipe
  if (mkfifo(register_fifo, 0640) != 0) {
    pthread_mutex_lock(&mutex_terminal);
    fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
    pthread_mutex_unlock(&mutex_terminal);
    return 1;
  }

  // Opens pipe for reading, this waits for clients to open it for writing
  int rx = open(register_fifo, O_RDWR);  // Read and Write to make sure it never closes
  if (rx == -1) {
    pthread_mutex_lock(&mutex_terminal);
    fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
    pthread_mutex_unlock(&mutex_terminal);
    return 1;
  }

  //Intializes server

  DynamicBuffer *prod_cons_buffer = createDynamicBuffer();
  if (prod_cons_buffer == NULL) {return 1;}

  ThreadData *threads = (ThreadData*)malloc(MAX_SESSION_COUNT * sizeof(ThreadData));
  if (threads == NULL) {
    print_error("Failed to allocate memory for ThreadData\n");
    return 1;
  }

  // Creates worker threads
  if (createThreads(threads, prod_cons_buffer) != 0) {
    print_error("Failed to create threads.\n");
    return 1;
  }

  while(1) {

    char req_pipe_path[MAX_FIFO_PATHNAME];
    char resp_pipe_path[MAX_FIFO_PATHNAME];

    if (sigusr1_received) {
      // Prints information and resets the flag
      if (print_info() != 0) {
        print_error("Error in printing requested information\n");
        return 1;
      }
      sigusr1_received = 0;
    }

    // It waits here for a client to send a setup request in the server's pipe
    int parse_setup_value = parse_setup(rx, req_pipe_path, resp_pipe_path);

    if (parse_setup_value == 1) {
      print_error("Invalid setup. See HELP for usage\n");
      return 1;
    }

    if (parse_setup_value == SIGNAL_DETECTED) {   // If the function detected a signal
      continue;
    }

    // Creates the new session and adds it to the producer-consumer queue
    if (make_session_request(req_pipe_path, resp_pipe_path, prod_cons_buffer) != 0) {
      print_error("Failed to setup\n");
      return 1;
    }

  }
}