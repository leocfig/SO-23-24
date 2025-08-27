#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>


#include "operations.h"
#include "queue_operations.h"
#include "common/constants.h"




// Function to create the dynamic buffer
DynamicBuffer *createDynamicBuffer() {

    DynamicBuffer *buffer = (DynamicBuffer*) malloc (sizeof(DynamicBuffer));
    if (!buffer) return NULL;

    buffer->head = NULL;
    buffer->tail = NULL;

    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        free(buffer);
        return NULL;
    }

    if (pthread_cond_init(&buffer->empty, NULL) != 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}


// Function to add a session request to the buffer
int addSessionRequest(DynamicBuffer *buffer, Session *session) {

    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        print_error("Error: Unable to allocate memory for a new node\n");
        return 1;
    }

    newNode->session = session;
    newNode->next = NULL;

    if (pthread_mutex_lock(&buffer->mutex) != 0) {
        print_error("Error locking buffer\n");
        return 1;
    }

    newNode->prev = buffer->tail;

    if (buffer->head == NULL) { // If the buffer is empty
        buffer->head = newNode;
        buffer->tail = newNode;

    } else { // If the buffer is not empty
        buffer->tail->next = newNode;
        buffer->tail = newNode;
    }

    // Notify that the queue is not empty
    pthread_cond_broadcast(&buffer->empty);

    if (pthread_mutex_unlock(&buffer->mutex) != 0) {
        print_error("Error unlocking active_sessions_mutex\n");
        return 1;
    }
    return 0;
}

// Function to remove and get the last session request from the buffer
Session *retrieveLastSessionRequest(DynamicBuffer *buffer) {

    if (pthread_mutex_lock(&buffer->mutex) != 0) {
        print_error("Error locking buffer\n");
        return NULL;
    }

    while (buffer->tail == NULL) {
        // Buffer is empty, wait for a session request
        pthread_cond_wait(&buffer->empty, &buffer->mutex);
    }

    Node *lastNode = buffer->tail;

    // Update the tail to the previous node
    buffer->tail = lastNode->prev;

    // If there is only one element in the list, update the head
    if (buffer->tail == NULL) {
        buffer->head = NULL;
    } 
    else {
        buffer->tail->next = NULL;
    }

    Session *session = lastNode->session;
    free(lastNode);

    if (pthread_mutex_unlock(&buffer->mutex) != 0) {
        print_error("Error unlocking active_sessions_mutex\n");
        return NULL;
    }

    return session;
}


// Function to clean up resources
void free_DynamicBuffer(DynamicBuffer *buffer) {

    if (!buffer) return;

    Node *current = buffer->head;
    while (current) {
        Node *temp = current;
        current = current->next;
        
        free(temp->session);
        free(temp);
    }

    pthread_mutex_destroy(&buffer->mutex);
    free(buffer);
}


Session *createSession(char req_pipe_path[], char resp_pipe_path[]) {
    
    Session *session = (Session*)malloc(sizeof(Session));
    if (session == NULL) {
      print_error("Failed to allocate memory for session\n");
      return NULL;
    }

    strcpy(session->req_pipe_path, req_pipe_path);
    strcpy(session->resp_pipe_path, resp_pipe_path);

    return session;
}

int closeSession(Session *session) {

    if (session != NULL) {

        // Free the memory associated with the session
        free(session);
        return 0;
    }
    return 1;
}





