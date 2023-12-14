#ifndef EMS_OPERATIONS_H
#define EMS_OPERATIONS_H

#include <stddef.h>
#include <pthread.h>


typedef struct WaitOrder WaitOrder;

struct WaitOrder {
  unsigned int delay;
  WaitOrder* next;
};

typedef struct{
  WaitOrder* first;
  WaitOrder* last;
} WaitListNode;

typedef struct {
  pthread_t threadId;
  int fileDescriptorIn;
  int fileDescriptorOut;
  int vector_position;
  int max_threads;
  WaitListNode *wait_vector;
} ThreadData;


/// Initializes the EMS state.
/// @param delay_ms State access delay in milliseconds.
/// @return 0 if the EMS state was initialized successfully, 1 otherwise.
int ems_init(unsigned int delay_ms);

/// Destroys the EMS state.
int ems_terminate();

/// Creates a new event with the given id and dimensions.
/// @param event_id Id of the event to be created.
/// @param num_rows Number of rows of the event to be created.
/// @param num_cols Number of columns of the event to be created.
/// @return 0 if the event was created successfully, 1 otherwise.
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols);

/// Creates a new reservation for the given event.
/// @param event_id Id of the event to create a reservation for.
/// @param num_seats Number of seats to reserve.
/// @param xs Array of rows of the seats to reserve.
/// @param ys Array of columns of the seats to reserve.
/// @return 0 if the reservation was created successfully, 1 otherwise.
int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys);

//Com
void sort_seats(size_t x[], size_t y[], size_t num_seats);

/// Prints the given event.
/// @param fd File descriptor to write in.
/// @param event_id Id of the event to print.
/// @return 0 if the event was printed successfully, 1 otherwise.
int ems_show(int fdOut, unsigned int event_id);

/// Prints all the events.
/// @param fd File descriptor to write in.
/// @return 0 if the events were printed successfully, 1 otherwise.
int ems_list_events(int fdOut);

/// Waits for a given amount of time.
/// @param delay_us Delay in milliseconds.
void ems_wait(unsigned int delay_ms);

/// Auxiliary function for the wait command
/// @param wait_vector Vector for every thread's wait orders
/// @param delay Delay in milliseconds.
/// @param index Index
/// @param rwl_wait The lock for the command
void addWaitOrder(WaitListNode* wait_vector, unsigned int delay, unsigned int index,pthread_mutex_t rwl_wait );

#endif  // EMS_OPERATIONS_H
