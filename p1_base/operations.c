#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "eventlist.h"
#include "operations.h"
#include "fileOperations.h"


pthread_rwlock_t rwl_create = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t rwl_reserve_and_show = PTHREAD_RWLOCK_INITIALIZER;


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
/// @return Pointer to the seat struct.
static struct Seat* get_seat_with_delay(struct Event* event, size_t index) {
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

  pthread_rwlock_rdlock(&rwl_create);

  if (get_event_with_delay(event_id) != NULL) {
    fprintf(stderr, "Event already exists\n");
    return 1;
  }

  pthread_rwlock_unlock(&rwl_create);

  struct Event* event = malloc(sizeof(struct Event));

  if (event == NULL) {
    fprintf(stderr, "Error allocating memory for event\n");
    return 1;
  }

  event->id = event_id;
  event->rows = num_rows;
  event->cols = num_cols;
  event->reservations = 0;
  event->data = malloc(num_rows * num_cols * sizeof(struct Seat));
  pthread_rwlock_init(&event->lock, NULL);

  if (event->data == NULL) {
    fprintf(stderr, "Error allocating memory for event data\n");
    free(event);
    return 1;
  }

  for (size_t i = 0; i < num_rows * num_cols; i++) {
    event->data[i].reservation_id = (unsigned int*)malloc(sizeof(unsigned int));
    if (event->data[i].reservation_id == NULL) {
      fprintf(stderr, "Error allocating memory for reservation_id\n");
      return 1;
    }
    *(event->data[i].reservation_id) = 0;
    pthread_rwlock_init(&event->data[i].lock, NULL);
  }

  pthread_rwlock_wrlock(&rwl_create);
  
  if (append_to_list(event_list, event) != 0) {
    fprintf(stderr, "Error appending event to list\n");
    for (size_t i = 0; i < event->rows * event->cols; i++) {
      pthread_rwlock_destroy(&event->data[i].lock);
      free(event->data[i].reservation_id);
    }
    pthread_rwlock_destroy(&event->lock);
    free(event->data);
    free(event);
    pthread_rwlock_unlock(&rwl_create);
    return 1;
  }
  pthread_rwlock_unlock(&rwl_create);
  return 0;
}

void sort_seats(size_t x[], size_t y[], size_t num_seats) {
  for (size_t i = 0; i < num_seats - 1; i++) {
    for (size_t j = 0; j < num_seats - i - 1; j++) {
      if (x[j] > x[j + 1] || (x[j] == x[j + 1] && y[j] > y[j + 1])) {
        // Swap x
        size_t temp = x[j];
        x[j] = x[j + 1];
        x[j + 1] = temp;

        // Swap y
        temp = y[j];
        y[j] = y[j + 1];
        y[j + 1] = temp;

      }
    }
  }
}

int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  pthread_rwlock_rdlock(&rwl_create);
  struct Event* event = get_event_with_delay(event_id);
  pthread_rwlock_unlock(&rwl_create);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    return 1;
  }

  sort_seats(xs, ys, num_seats);
  struct Seat* reservation_seats[num_seats];

  size_t i = 0;
  for (; i < num_seats; i++) {
    size_t row = xs[i];
    size_t col = ys[i];

    if (row <= 0 || row > event->rows || col <= 0 || col > event->cols) {
      fprintf(stderr, "Invalid seat\n");
      break;
    }

    struct Seat* seat = get_seat_with_delay(event, seat_index(event, row, col));

    pthread_rwlock_wrlock(&seat->lock);

    if (*seat->reservation_id != 0) {
      fprintf(stderr, "Seat already reserved\n");
      pthread_rwlock_unlock(&seat->lock);
      break;
    }

    reservation_seats[i]=seat;
  }

  // If the reservation was not successful, free the seats that were reserved.
  if (i < num_seats) {
    for (size_t j = 0; j < i; j++) {
      struct Seat* seat = get_seat_with_delay(event, seat_index(event, xs[j], ys[j]));
      pthread_rwlock_unlock(&seat->lock);
    }
    return 1;
  }

  i = 0;
  pthread_rwlock_wrlock(&event->lock);                            // ---> isto está bem? E podemos mudar para mutex né, nunca fazemos read
  unsigned int reservation_id = ++event->reservations;
  pthread_rwlock_unlock(&event->lock);

  for (; i < num_seats; i++) {
    *reservation_seats[i]->reservation_id=reservation_id;
    pthread_rwlock_unlock(&reservation_seats[i]->lock);
  }

  return 0;
}

int ems_show(int fdOut, unsigned int event_id) {
  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  pthread_rwlock_rdlock(&rwl_create);
  struct Event* event = get_event_with_delay(event_id);
  pthread_rwlock_unlock(&rwl_create);

  if (event == NULL) {
    fprintf(stderr, "Event not found\n");
    return 1;
  }

  size_t nr_cols = event->cols; // To get the number of seats in a row
  unsigned int *row = (unsigned int*)malloc(nr_cols * sizeof(unsigned int));

  if (row == NULL) {
    fprintf(stderr, "Memory allocation for row failed\n");
    return 1;
  }

  char *char_buffer = (char *)malloc((event->rows * nr_cols * (UNS_INT_SIZE + 1) + 1 )* sizeof(char));
  if (char_buffer == NULL) {
    fprintf(stderr, "Memory allocation for char_buffer failed\n");
    return 1;
  }
  char_buffer[0]=0;

  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1, k = 0; j <= event->cols; j++) {

      struct Seat* seat = get_seat_with_delay(event, seat_index(event, i, j));
      pthread_rwlock_rdlock(&seat->lock);
      row[k++] = *seat->reservation_id;
    }
    char *buffer = buffer_to_string(row, nr_cols, SHOW_KEY);
    strcat(char_buffer,buffer);
    free(buffer);
  }

  for (size_t i = 1; i <= event->rows; i++) {
    for (size_t j = 1; j <= event->cols; j++) {

      struct Seat* seat = get_seat_with_delay(event, seat_index(event, i, j));
      pthread_rwlock_unlock(&seat->lock);
    }
  }

  write_inFile(fdOut, char_buffer);
  free(row);
  free(char_buffer);
  return 0;
}

int ems_list_events(int fdOut) {

  if (event_list == NULL) {
    fprintf(stderr, "EMS state must be initialized\n");
    return 1;
  }

  pthread_rwlock_rdlock(&rwl_create);

  if (event_list->head == NULL) {
    write_inFile(fdOut, "No events\n");
    pthread_rwlock_unlock(&rwl_create);
    return 0;
  }
  
  // 8 is to allocate enough memory for the string "Event: " plus the "\n"
  char* buffer_list=(char*)malloc((event_list->total_events*(8+UNS_INT_SIZE)+1)*sizeof(char));
  if (buffer_list == NULL) {
    fprintf(stderr, "Memory allocation for buffer_list failed\n");
    pthread_rwlock_unlock(&rwl_create);
    return 1;
  }
  buffer_list[0]=0;

  struct ListNode* current = event_list->head;
  while (current != NULL) {
    strcat(buffer_list, "Event: ");
    unsigned int *event_ID = (unsigned int*)malloc(sizeof(unsigned int) + 1);
    if (event_ID == NULL) {
      fprintf(stderr, "Memory allocation for event_ID failed\n");
      pthread_rwlock_unlock(&rwl_create);
      return 1;
    }
    event_ID[0] = (current->event)->id;
    char *buffer = buffer_to_string(event_ID, 1, LIST_KEY);
    strcat(buffer_list,buffer);
    free(buffer);
    free(event_ID);
    current = current->next;
  }
  pthread_rwlock_unlock(&rwl_create);
  write_inFile(fdOut, buffer_list);
  free(buffer_list);
  
  return 0;
}

void ems_wait(unsigned int delay_ms) {
  struct timespec delay = delay_to_timespec(delay_ms);
  nanosleep(&delay, NULL);
}



