#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#include "common/io.h"
#include "common/constants.h"
#include "eventlist.h"
#include "queue_operations.h"



static struct EventList* event_list = NULL;
static unsigned int state_access_delay_us = 0;

pthread_mutex_t mutex_terminal = PTHREAD_MUTEX_INITIALIZER;


int active_sessions = 0;
pthread_mutex_t active_sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t session_terminated_cond = PTHREAD_COND_INITIALIZER;


/// Print to the stderr a simple error mesage
/// @param error Mensage to be sent
void print_error(const char *error) {
  if (pthread_mutex_lock(&mutex_terminal) != 0) {
    fprintf(stderr, "Error locking mutex_terminal\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "%s", error);
  if (pthread_mutex_unlock(&mutex_terminal) != 0) {
    fprintf(stderr, "Error unlocking mutex_terminal\n");
    exit(EXIT_FAILURE);
  }
}

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event_id The ID of the event to get.
/// @param from First node to be searched.
/// @param to Last node to be searched.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event* get_event_with_delay(unsigned int event_id, struct ListNode* from, struct ListNode* to) {
  struct timespec delay = {0, state_access_delay_us * 1000};
  nanosleep(&delay, NULL);  // Should not be removed

  return get_event(event_list, event_id, from, to);
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event* event, size_t row, size_t col) { return (row - 1) * event->cols + col - 1; }

int ems_init(unsigned int delay_us) {
  if (event_list != NULL) {
    print_error("EMS state has already been initialized\n");
    return 1;
  }

  event_list = create_list();
  state_access_delay_us = delay_us;

  return event_list == NULL;
}

int ems_terminate(DynamicBuffer *buffer, ThreadData *threads) {

  if (event_list == NULL) {
    print_error("EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_wrlock(&event_list->rwl) != 0) {
    print_error("Error locking event list rwl\n");
    return 1;
  }

  free_list(event_list);
  if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
    print_error("Error unlocking event list rwl\n");
    return 1;
  }

  if (pthread_mutex_lock(&buffer->mutex) != 0) {
    print_error("Error locking request list rwl\n");
    return 1;
  }

  free_DynamicBuffer(buffer);
  if (pthread_mutex_unlock(&buffer->mutex) != 0) {
    print_error("Error unlocking event list rwl\n");
    return 1;
  }

  free(threads);

  return 0;
}


int make_session_request(char *req_pipe_path, char *resp_pipe_path,
                        DynamicBuffer *buffer) {

  if (pthread_mutex_lock(&active_sessions_mutex) != 0) {
    print_error("Error locking active_sessions_mutex\n");
    return 1;
  }

  // Waits for a session to terminate if all sessions are active 
  while (active_sessions == MAX_SESSION_COUNT) {
    pthread_cond_wait(&session_terminated_cond, &active_sessions_mutex);
  }

  if (pthread_mutex_unlock(&active_sessions_mutex) != 0) {
    print_error("Error unlocking active_sessions_mutex\n");
    return 1;
  }

  Session *session = createSession(req_pipe_path, resp_pipe_path);
  if (session == NULL)
    return 1;

  // Writes new client to the producer-consumer buffer
  if (addSessionRequest(buffer, session) != 0) {return 1;}

  return 0;
}

int ems_setup(Session *session) {

  char *req_pipe_path = session->req_pipe_path;
  char *resp_pipe_path = session->resp_pipe_path;
  int session_id = session->session_id;

  // Opens req_pipe_path
  int req_pipe = open(session->req_pipe_path, O_RDONLY);
  if (req_pipe == -1) {
    pthread_mutex_lock(&mutex_terminal);
    fprintf(stderr, "[ERR]: open of %s failed: %s\n", req_pipe_path, strerror(errno));
    pthread_mutex_unlock(&mutex_terminal);
    return 1;
  }

  // Opens resp_pipe_path
  int resp_pipe = open(session->resp_pipe_path, O_WRONLY);
  if (resp_pipe == -1) {
    pthread_mutex_lock(&mutex_terminal);
    fprintf(stderr, "[ERR]: open of %s failed: %s\n", resp_pipe_path, strerror(errno));
    pthread_mutex_unlock(&mutex_terminal);
    return 1;
  }

  if (pthread_mutex_lock(&active_sessions_mutex) != 0) {
    print_error("Error locking active_sessions_mutex\n");
    return 1;
  }

  // This session is now active
  active_sessions++;

  if (pthread_mutex_unlock(&active_sessions_mutex) != 0) {
    print_error("Error unlocking active_sessions_mutex\n");
    return 1;
  }

  // Returns the session ID to the client
  int print_value = print_int_pipe(resp_pipe, session_id);
  if (print_value == 1) {return 1;}
  if (print_value == PIPE_CLOSED) {
    return PIPE_CLOSED;
  }

  session->req_pipe = req_pipe;
  session->resp_pipe = resp_pipe;

  return 0;
}


int ems_quit(Session *session) {

  // Closes the pipes and session
  close(session->req_pipe);
  close(session->resp_pipe);
  if (closeSession(session) != 0) {return 1;}

  if (pthread_mutex_lock(&active_sessions_mutex) != 0) {
    print_error("Error locking active_sessions_mutex\n");
    return 1;
  }

  active_sessions--;
  // Signal that a session has terminated
  pthread_cond_signal(&session_terminated_cond);

  if (pthread_mutex_unlock(&active_sessions_mutex) != 0) {
    print_error("Error unlocking active_sessions_mutex\n");
    return 1;
  }

  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {

  if (event_list == NULL) {
    print_error("EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_wrlock(&event_list->rwl) != 0) {
    print_error("Error locking list rwl\n");
    return 1;
  }

  if (get_event_with_delay(event_id, event_list->head, event_list->tail) != NULL) {
    print_error("Event already exists\n");
    pthread_rwlock_unlock(&event_list->rwl);
    return 1;
  }

  struct Event* event = malloc(sizeof(struct Event));

  if (event == NULL) {
    print_error("Error allocating memory for event\n");
    pthread_rwlock_unlock(&event_list->rwl);
    return 1;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  if (pthread_mutex_init(&event->mutex, NULL) != 0) {
    pthread_rwlock_unlock(&event_list->rwl);
    free(event);
    return 1;
  }

  event->data = calloc(num_rows * num_cols, sizeof(unsigned int));

  if (event->data == NULL) {
    print_error("Error allocating memory for event data\n");
    pthread_rwlock_unlock(&event_list->rwl);
    free(event);
    return 1;
  }

  if (append_to_list(event_list, event) != 0) {
    print_error("Error appending event to list\n");
    pthread_rwlock_unlock(&event_list->rwl);
    free(event->data);
    free(event);
    return 1;
  }

  event_list->num_events++;
  if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
    print_error("Error unlocking event list rwl\n");
    return 1;
  }

  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t* xs, size_t* ys) {
  if (event_list == NULL) {
    print_error("EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    print_error("Error locking list rwl\n");
    return 1;
  }

  struct Event* event = get_event_with_delay(event_id, event_list->head, event_list->tail);

  if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
    print_error("Error unlocking event list rwl\n");
    return 1;
  }

  if (event == NULL) {
    print_error("Event not found\n");
    return 1;
  }

  if (pthread_mutex_lock(&event->mutex) != 0) {
    print_error("Error locking mutex\n");
    return 1;
  }

  for (size_t i = 0; i < num_seats; i++) {
    if (xs[i] <= 0 || xs[i] > event->rows || ys[i] <= 0 || ys[i] > event->cols) {
      print_error("Seat out of bounds\n");
      pthread_mutex_unlock(&event->mutex);
      return 1;
    }
  }

  for (size_t i = 0; i < event->rows * event->cols; i++) {
    for (size_t j = 0; j < num_seats; j++) {
      if (seat_index(event, xs[j], ys[j]) != i) {
        continue;
      }

      if (event->data[i] != 0) {
        print_error("Seat already reserved\n");
        pthread_mutex_unlock(&event->mutex);
        return 1;
      }

      break;
    }
  }

  unsigned int reservation_id = ++event->reservations;

  for (size_t i = 0; i < num_seats; i++) {
    event->data[seat_index(event, xs[i], ys[i])] = reservation_id;
  }

  if (pthread_mutex_unlock(&event->mutex) != 0) {
    print_error("Error unlocking mutex\n");
    return 1;
  }

  return 0;
}

int ems_show(unsigned int event_id, unsigned int **seats, size_t *num_rows, size_t *num_cols) {

  if (event_list == NULL) {
    print_error("EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    print_error("Error locking list rwl\n");
    return 1;
  }

  struct Event* event = get_event_with_delay(event_id, event_list->head, event_list->tail);

  if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
    print_error("Error unlocking event list rwl\n");
    return 1;
  }

  if (event == NULL) {
    print_error("Event not found\n");
    return 1;
  }

  if (pthread_mutex_lock(&event->mutex) != 0) {
    print_error("Error locking mutex\n");
    return 1;
  }

  *seats = (unsigned int*)malloc(sizeof(unsigned int) * event->cols * event->rows);
  if (*seats == NULL){
    print_error("Error allocating the event seats\n");
    return 1;
  }

  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1; j <= event->cols; j++) {
      size_t index = (i-1) * event->cols + (j-1);
      (*seats)[index] = event->data[seat_index(event, i, j)];
    }
  }
  *num_rows = event->rows;
  *num_cols = event->cols;
  if (pthread_mutex_unlock(&event->mutex) != 0) {
    print_error("Error unlocking mutex\n");
    return 1;
  }

  return 0;
}

int ems_list_events(unsigned int **event_ids, size_t *num_events) {
  if (event_list == NULL) {
    print_error("EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    print_error("Error locking list rwl\n");
    return 1;
  }

  struct ListNode* to = event_list->tail;
  struct ListNode* current = event_list->head;

  if (current == NULL) {
    if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
      print_error("Error unlocking event list rwl\n");
      return 1;
    }
    *num_events = 0;
    return 0;
  }

  *event_ids = (unsigned int*) malloc(sizeof(unsigned int) * event_list->num_events);
  if (*event_ids == NULL){
    print_error("Error allocating the event ids\n");
    return 1;
  }

  size_t k = 0;

  while (1) {
    (*event_ids)[k] = (current->event)->id;
    if (current == to) {
      break;
    }
    current = current->next;
    k++;
  }

  *num_events = event_list->num_events;
  if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
    print_error("Error unlocking event list rwl\n");
    return 1;
  }
  return 0;
}


// Function to print events informations
int print_info() {

  if (event_list == NULL) {
    print_error("EMS state must be initialized\n");
    return 1;
  }

  if (pthread_rwlock_rdlock(&event_list->rwl) != 0) {
    print_error("Error locking list rwl\n");
    return 1;
  }

  struct ListNode* to = event_list->tail;
  struct ListNode* current = event_list->head;

  if (pthread_mutex_lock(&mutex_terminal) != 0) {
    print_error("Error locking mutex_terminal\n");
    return 1;
  }

  if (current == NULL) {
    if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
      print_error("Error unlocking event list rwl\n");
      return 1;
    }
    if (pthread_mutex_unlock(&mutex_terminal) != 0) {
      print_error("Error unlocking mutex_terminal\n");
      return 1;
    }
    return 0; // There are no events created
  }

  unsigned int *event_seats = NULL;

  while (1) {

    size_t rows;
    size_t cols;

    fprintf(stdout, "Event: %u\n", (current->event)->id);
    if (ems_show((current->event)->id, &event_seats, &rows, &cols) != 0) {
      pthread_rwlock_unlock(&event_list->rwl);
      pthread_mutex_unlock(&mutex_terminal);
      return 1; 
    }
    if (print_output_show(STDOUT, (current->event)->rows, (current->event)->cols, event_seats) != 0) {
      pthread_rwlock_unlock(&event_list->rwl);
      pthread_mutex_unlock(&mutex_terminal);
      return 1;
    }

    if (current == to) {
      break;
    }
    current = current->next;
  }
  if (pthread_rwlock_unlock(&event_list->rwl) != 0) {
    fprintf(stderr, "Error unlocking event list rwl\n");
    return 1;
  }
  if (pthread_mutex_unlock(&mutex_terminal) != 0) {
    print_error("Error unlocking mutex_terminal\n");
    return 1;
  }
  return 0;
}
