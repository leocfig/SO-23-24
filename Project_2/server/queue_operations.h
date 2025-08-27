#ifndef QUEUE_OPERATIONS_H
#define QUEUE_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


#include "operations.h"

typedef struct struct_session Session;

// Structure for a node in the linked list
typedef struct Node {
  Session *session;
  struct Node *next;
  struct Node *prev;
} Node;

// Structure for the dynamic buffer
typedef struct dynamicBuffer {
  Node *head;
  Node *tail;
  pthread_mutex_t mutex;
  pthread_cond_t empty;
} DynamicBuffer;


typedef struct {
  pthread_t threadId;
  DynamicBuffer *buffer;
  int session_id;
} ThreadData;

/// Creates a dynamic buffer to store the clients setup requests.
/// @return The pointer to the created buffer.
DynamicBuffer *createDynamicBuffer();

/// Adds a session to the buffer as a setup request.
/// @param buffer The pointer to the buffer to add the request.
/// @param session The pointer to the session to be added to the buffer.
/// @return 0 if the request was added successfully, 1 otherwise.
int addSessionRequest(DynamicBuffer *buffer, Session *session);

/// Retrieves the last session from the buffer for it to be activated.
/// @param buffer The buffer to get the request from.
/// @return Pointer to the session to be initialized.
Session *retrieveLastSessionRequest(DynamicBuffer *buffer);

/// Deallocates all the memory associated with the buffer.
/// @param buffer The buffer to be deallocated.
void free_DynamicBuffer(DynamicBuffer *buffer);

/// Creates a session/setup request.
/// @param req_pipe_path The request pipe path of the new session.
/// @param resp_pipe_path The response pipe path of the new session.
/// @return Pointer to the session created.
Session *createSession(char req_pipe_path[], char resp_pipe_path[]);

/// Closes an active session by deallocatting all the memory associated with it.
/// @param session The session to be closed.
/// @return 0 if the session was closed successfully, 1 otherwise.
int closeSession(Session *session);

#endif