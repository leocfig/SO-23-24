#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <time.h>
#include <pthread.h>

#include "eventlist.h"
#include "operations.h"
#include "fileOperations.h"


//pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;

pthread_rwlock_t rwl_1 = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rwl_2 = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rwl_3 = PTHREAD_RWLOCK_INITIALIZER;


static struct EventList* event_list = NULL;
static unsigned int state_access_delay_ms = 0;

/// Calculates a timespec from a delay in milliseconds.
/// @param delay_ms Delay in milliseconds.
/// @return Timespec with the given delay.
static struct timespec delay_to_timespec(unsigned int delay_ms) {
  return (struct timespec){delay_ms / 1000, (delay_ms % 1000) * 1000000};
}

/// Gets the event with the given ID from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event_id The ID of the event to get.
/// @return Pointer to the event if found, NULL otherwise.
static struct Event* get_event_with_delay(unsigned int event_id) {
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL);  // Should not be removed

  return get_event(event_list, event_id);
}

/// Gets the seat with the given index from the state.
/// @note Will wait to simulate a real system accessing a costly memory resource.
/// @param event Event to get the seat from.
/// @param index Index of the seat to get.
/// @return Pointer to the seat.
static unsigned int* get_seat_with_delay(struct Event* event, size_t index) {
  struct timespec delay = delay_to_timespec(state_access_delay_ms);
  nanosleep(&delay, NULL);  // Should not be removed

  return &event->data[index];
}

/// Gets the index of a seat.
/// @note This function assumes that the seat exists.
/// @param event Event to get the seat index from.
/// @param row Row of the seat.
/// @param col Column of the seat.
/// @return Index of the seat.
static size_t seat_index(struct Event* event, size_t row, size_t col) { return (row - 1) * event->cols + col - 1; }

int ems_init(unsigned int delay_ms) {
  if (event_list != NULL) {
    fprintf(stderr, "EMS state has already been initialized\n");
    return 1;
  }

  event_list = create_list();
  state_access_delay_ms = delay_ms;

  return event_list == NULL;
}

int ems_terminate() {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  free_list(event_list);
  return 0;
}

int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  pthread_mutex_lock(&mutex_3);

  //pthread_rwlock_rdlock(&rwl);
  if (get_event_with_delay(event_id) != NULL) {
    fprintf(stderr, "Event already exists\n");
    pthread_mutex_unlock(&mutex_3);
    return 1;
  }
  //pthread_rwlock_unlock(&rwl);

  struct Event* event = malloc(sizeof(struct Event));

  if (event == NULL) {
    fprintf(stderr, "Error allocating memory for event\n");
    pthread_mutex_unlock(&mutex_3);
    return 1;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  event->data = malloc(num_rows * num_cols * sizeof(unsigned int));

  if (event->data == NULL) {
    fprintf(stderr, "Error allocating memory for event data\n");
    free(event);
    pthread_mutex_unlock(&mutex_3);
    return 1;
  }

  for (size_t i = 0; i < num_rows * num_cols; i++) {
    event->data[i] = 0;
  }

  //pthread_rwlock_rdlock(&rwl);
  if (append_to_list(event_list, event) != 0) {
    fprintf(stderr, "Error appending event to list\n");
    free(event->data);
    free(event);
    //pthread_rwlock_unlock(&rwl);
    pthread_mutex_unlock(&mutex_3);
    return 1;
  }

  pthread_mutex_unlock(&mutex_3);
  //printf("Events appended: %d\n", event->id);
  //pthread_rwlock_unlock(&rwl);
  return 0;
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  //pthread_mutex_lock(&mutex_1);

  pthread_rwlock_rdlock(&rwl_1);
  struct Event* event = get_event_with_delay(event_id);
  pthread_rwlock_unlock(&rwl_1);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    //pthread_mutex_unlock(&mutex_1);
    return 1;
  }

  pthread_rwlock_wrlock(&rwl_1);
  unsigned int reservation_id = ++event->reservations;

  size_t i = 0;
  for (; i < num_seats; i++) {
    size_t row = xs[i];
    size_t col = ys[i];

    if (row <= 0 || row > event->rows || col <= 0 || col > event->cols) {
      fprintf(stderr, "Invalid seat\n");
      break;
    }

    if (*get_seat_with_delay(event, seat_index(event, row, col)) != 0) {
      fprintf(stderr, "Seat already reserved\n");
      break;
    }

    *get_seat_with_delay(event, seat_index(event, row, col)) = reservation_id;
  }

  // If the reservation was not successful, free the seats that were reserved.
  if (i < num_seats) {
    event->reservations--;
    for (size_t j = 0; j < i; j++) {
      *get_seat_with_delay(event, seat_index(event, xs[j], ys[j])) = 0;
    }
    //pthread_mutex_unlock(&mutex_1);
    pthread_rwlock_unlock(&rwl_1);
    return 1;
  }

  pthread_rwlock_unlock(&rwl_1);
  //pthread_mutex_unlock(&mutex_1);
  return 0;
}

int ems_show(int fdOut, unsigned int event_id) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  //pthread_mutex_lock(&mutex_1);
  pthread_rwlock_wrlock(&rwl_2);

  pthread_rwlock_rdlock(&rwl_1);
  struct Event* event = get_event_with_delay(event_id);
  pthread_rwlock_unlock(&rwl_1);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    //pthread_mutex_unlock(&mutex_1);
    pthread_rwlock_unlock(&rwl_2);
    return 1;
  }

  size_t row_size = event->cols; // pôr comentário ...
  unsigned int *row = (unsigned int*)malloc(row_size * sizeof(unsigned int));

  if (row == NULL) {
    fprintf(stderr, "Memory allocation for row failed\n");
    //pthread_mutex_unlock(&mutex_1);
    pthread_rwlock_unlock(&rwl_2);
    return 1;
  }

  pthread_rwlock_wrlock(&rwl_1);
  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1, k = 0; j <= event->cols; j++) {

      unsigned int* seat = get_seat_with_delay(event, seat_index(event, i, j));
      row[k++] = *seat;
    }
    char *buffer = buffer_to_string(row, row_size, SHOW_KEY);
    write_inFile(fdOut, buffer);
    free(buffer);
  }
  pthread_rwlock_unlock(&rwl_1);
  free(row);
  //pthread_mutex_unlock(&mutex_1);
  pthread_rwlock_unlock(&rwl_2);
  return 0;
}

int ems_list_events(int fdOut) {

  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  if (event_list->head == NULL) {
    write_inFile(fdOut, "No events\n");
    return 0;
  }
  
  pthread_rwlock_wrlock(&rwl_2);

  struct ListNode* current = event_list->head;
  while (current != NULL) {
    write_inFile(fdOut, "Event: ");
    unsigned int *event_ID = (unsigned int*)malloc(sizeof(unsigned int) + 1);
    event_ID[0] = (current->event)->id;
    char *buffer = buffer_to_string(event_ID, 1, LIST_KEY);
    write_inFile(fdOut, buffer);
    free(buffer);
    free(event_ID);
    current = current->next;
  }
  
  pthread_rwlock_unlock(&rwl_2);
  return 0;
}

void ems_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}


void addWaitOrder(WaitListNode* wait_vector, unsigned int delay, unsigned int index) {
  // colocar locks 
  WaitOrder *wait = (WaitOrder*)malloc(sizeof(WaitOrder));
  if (wait_vector == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
  }

  wait->delay = delay;
  wait->next = NULL;

  if (wait_vector[index].first == NULL) {
    wait_vector[index].first = wait;
  } 
  else {
    wait_vector[index].last->next = wait;
  }

  wait_vector[index].last = wait;
}
