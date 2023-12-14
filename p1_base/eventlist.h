#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include <stddef.h>
#include <pthread.h>

struct Seat {
  unsigned int* reservation_id;  // Reservation ID for the seat
  pthread_rwlock_t lock;        // Read-write lock for the seat
};

struct Event {
  unsigned int id;                      /// Event id
  unsigned int reservations;            /// Number of reservations for the event.

  size_t cols;  /// Number of columns.
  size_t rows;  /// Number of rows.

  struct Seat* data;             /// Array of size rows * cols with the seats.
  pthread_rwlock_t lock_event;
};

struct ListNode {
  struct Event* event;
  struct ListNode* next;
};

// Linked list structure
struct EventList {
  struct ListNode* head;  // Head of the list
  struct ListNode* tail;  // Tail of the list
  unsigned int total_events; // Total number of events
};

/// Creates a new event list.
/// @return Newly created event list, NULL on failure
struct EventList* create_list();

/// Appends a new node to the list.
/// @param list Event list to be modified.
/// @param data Event to be stored in the new node.
/// @return 0 if the node was appended successfully, 1 otherwise.
int append_to_list(struct EventList* list, struct Event* data);

/// Removes a node from the list.
/// @param list Event list to be modified.
/// @return 0 if the node was removed successfully, 1 otherwise.
void free_list(struct EventList* list);

/// Retrieves an event in the list.
/// @param list Event list to be searched
/// @param event_id Event id.
/// @return Pointer to the event if found, NULL otherwise.
struct Event* get_event(struct EventList* list, unsigned int event_id);

#endif  // EVENT_LIST_H
