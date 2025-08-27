#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <stddef.h>

#include "common/constants.h"
#include "queue_operations.h"

// Mutex for the server's terminal
extern pthread_mutex_t mutex_terminal;

typedef struct dynamicBuffer DynamicBuffer;


typedef struct struct_session {
  int session_id;                           /// Session id
  char req_pipe_path[MAX_FIFO_PATHNAME];    /// Request client -> server 
  char resp_pipe_path[MAX_FIFO_PATHNAME];   /// Response server -> client
  int req_pipe;                             /// File descriptor request pipe
  int resp_pipe;                            /// File descriptor response pipe
} Session;


/// Initializes the EMS state.
/// @param delay_us Delay in microseconds.
/// @return 0 if the EMS state was initialized successfully, 1 otherwise.
int ems_init(unsigned int delay_us);

/// Destroys the EMS state.
int ems_terminate();

/// Adds a session request to the buffer.
/// @param req_pipe_path The filepath to the client's request pipe.
/// @param resp_pipe_path The filepath to the client's response pipe.
/// @param buffer The buffer to add the session request.
/// @return 0 if the request was successfully made, 1 otherwise.
int make_session_request(char *req_pipe_path, char *resp_pipe_path,
                        DynamicBuffer *buffer);

/// Writes to the STDERR an error message.
/// @param error The error mensage to be printed.
void print_error(const char * error);

/// Initializes the session using the associated named pipes.
/// @param session The to be initialized.
/// @return 0 if it was successfully made, 1 otherwise.
int ems_setup(Session *session);

/// Closes the active session.
/// @param session The session to be closed.
/// @return 0 if it was successfully made, 1 otherwise.
int ems_quit(Session *session);

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

/// Prints the given event.
/// @param event_id Id of the event to print.
/// @param seats Variable to store the seats matrix.
/// @param num_rows Variable to store the number of rows of the event.
/// @param num_cols Variable to store the number of rows of the event.
/// @return 0 if the event was printed successfully, 1 otherwise.
int ems_show(unsigned int event_id, unsigned int **seats, size_t *num_rows, size_t *num_cols);

/// Prints all the events.
/// @param event_ids Pointer to register the IDs of the existing events.
/// @param num_events Number of existing events.
/// @return 0 if the events were printed successfully, 1 otherwise.
int ems_list_events(unsigned int **event_ids, size_t *num_events);

/// Prints all the information about all the existing events. 
/// @return 0 if the events were printed successfully, 1 otherwise.
int print_info();


#endif  // SERVER_OPERATIONS_H
