#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdint.h>
#include <pthread.h>

#include "fileOperations.h"
#include "operations.h"
#include "constants.h"
#include "parser.h"



int barrier;  // Variável global
pthread_mutex_t mutex_wait = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_command = PTHREAD_MUTEX_INITIALIZER;

void* processCommand(void* arg) {
  unsigned int event_id, delay, thread_id;
  size_t num_rows, num_columns, num_coords;
  size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

  ThreadData* threadData = (ThreadData*)arg;

  int fileDescriptorIn = threadData->fileDescriptorIn;
  int fileDescriptorOut = threadData->fileDescriptorOut;
  int max_threads = threadData->max_threads;
  int vector_position = threadData->vector_position;
  WaitListNode *wait_vector = threadData->wait_vector;

  int endOfFile = 1;

  while (endOfFile) {

    if (barrier==1) pthread_exit((void*)BARRIER_EXIT);

    while (wait_vector[vector_position].first != NULL) {
      printf("Waiting...\n");
      ems_wait(wait_vector[vector_position].first->delay);

      pthread_mutex_lock(&mutex_wait);
      WaitOrder* new_first = wait_vector[vector_position].first->next;
      free(wait_vector[vector_position].first);
      wait_vector[vector_position].first = new_first;
      if (wait_vector[vector_position].first == NULL)
        wait_vector[vector_position].last = NULL;
      pthread_mutex_unlock(&mutex_wait);
    }

     pthread_mutex_lock(&mutex_command);

    switch (get_next(fileDescriptorIn)) {
      
      case CMD_CREATE:
        if (parse_create(fileDescriptorIn, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          pthread_mutex_unlock(&mutex_command);
          continue;
        }

        pthread_mutex_unlock(&mutex_command);

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }

        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(fileDescriptorIn, MAX_RESERVATION_SIZE, &event_id, xs, ys);
        pthread_mutex_unlock(&mutex_command);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(fileDescriptorIn, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          pthread_mutex_unlock(&mutex_command);
          continue;
        }

        pthread_mutex_unlock(&mutex_command);

        if (ems_show(fileDescriptorOut, event_id)) {
          fprintf(stderr, "Failed to show event\n");
        }
        break;

      case CMD_LIST_EVENTS:
        pthread_mutex_unlock(&mutex_command);

        if (ems_list_events(fileDescriptorOut)) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        int parse_result = parse_wait(fileDescriptorIn, &delay, &thread_id);
        pthread_mutex_unlock(&mutex_command);

        if (parse_result == -1) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          if (parse_result == 0) {  // no thread was specified
            for (int i = 0; i < max_threads; i++) { 
              addWaitOrder(wait_vector, delay, (unsigned)i, mutex_wait);
            }
          }
          else {                    // if a thread was specified
            if (thread_id > 0 && thread_id <= (unsigned)max_threads) {
              unsigned int index = thread_id - 1;
              addWaitOrder(wait_vector, delay, index, mutex_wait);
            }
          }
        }

        break;

      case CMD_INVALID:
        pthread_mutex_unlock(&mutex_command);
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        pthread_mutex_unlock(&mutex_command);
        
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  
            "  BARRIER\n"                      
            "  HELP\n");

        break;

      case CMD_BARRIER:
        pthread_mutex_unlock(&mutex_command);
        barrier = 1;
        pthread_exit((void*)BARRIER_EXIT);
        break;
      case CMD_EMPTY:
        pthread_mutex_unlock(&mutex_command);
        break;
      case EOC:
        pthread_mutex_unlock(&mutex_command);
        endOfFile=0;
        break;
    }
  }
  pthread_exit((void*)EXIT_SUCCESS);
}


int createThreads(ThreadData* threads[], int max_threads, int fileDescriptorIn, int fileDescriptorOut) {

  WaitListNode *wait_vector = (WaitListNode *)malloc((size_t)max_threads*sizeof(WaitListNode));
  if (wait_vector == NULL) 
    fprintf(stderr, "Failed to allocate memory\n");

  for (int i = 0; i < max_threads; i++) {
    wait_vector[i].first = NULL;
    wait_vector[i].last = NULL;
  }

  barrier=0;

  for (int i = 0; i < max_threads; i++) {
    threads[i] = (ThreadData*)malloc(sizeof(ThreadData));

    if (threads[i] == NULL) {
      fprintf(stderr, "Failed to allocate memory for ThreadData\n");

      // Clean up previously allocated memory
      for (int j = 0; j < i; j++) 
        free(threads[j]);

      return 1;  // To indicate failure
    }

    threads[i]->vector_position = i;
    threads[i]->fileDescriptorIn = fileDescriptorIn;
    threads[i]->fileDescriptorOut = fileDescriptorOut;
    threads[i]->max_threads = max_threads;
    threads[i]->wait_vector = wait_vector;

    pthread_create(&threads[i]->threadId, NULL, processCommand, (void*)threads[i]);
  }

  return 0;  // To indicate success
}


int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 4) {
    char *endptr;
    unsigned long int delay = strtoul(argv[4], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
  }
  if (argc <= 3) {
    fprintf(stderr, "Too few arguments\n");
    return 1;
  }

  if (ems_init(state_access_delay_ms)) {
    fprintf(stderr, "Failed to initialize EMS\n");
    return 1;
  }

  const char *dirpath = argv[1];
  int max_proc = atoi(argv[2]);
  int max_threads = atoi(argv[3]);
  
  DIR *dirp;
  struct dirent *dp;
  dirp = opendir(dirpath);

  if (dirp == NULL){
    fprintf(stderr, "opendir failed on: %s\n", dirpath);
    return 1;
  }

  pid_t pid;
  size_t sizePath = strlen(dirpath);
  int active_child_proc = 0;
  int status;
  int fileDescriptorIn, fileDescriptorOut;

  for (;;) {
    
    errno = 0;              // To distinguish error from end-of-directory

    while (active_child_proc < max_proc) {

      dp = readdir(dirp);
      if (dp == NULL)
        break;              // No more files in the directory

      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0 || 
        !has_extension(dp->d_name, ".jobs")) {
        /* Skips . and .. and files with other extensions other than ".jobs" */
        continue;
      }

      // +2 for the "/" and the "\0"
      char *filePath = (char *)malloc((sizePath + strlen(dp->d_name) + 2) * sizeof(char));

      if (filePath == NULL) {
        fprintf(stderr, "Memory allocation for filePath failed\n");
        return 1;
      }

      strcpy(filePath, dirpath);
      strcat(filePath, "/");

      fileDescriptorIn = open(strcat(filePath, dp->d_name), O_RDONLY, NULL);
      if (fileDescriptorIn < 0){
        fprintf(stderr, "Open error: %s\n", strerror(errno));
        return -1;
      }
      fileDescriptorOut = openFile(change_extension(filePath, "out"), 
                              O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
      if (fileDescriptorOut < 0){
        fprintf(stderr, "Open error: %s\n", strerror(errno));
        return -1;
      }

      free(filePath);

      pid = fork();
      active_child_proc++;
      
      if (pid == -1) {
        fprintf(stderr, "Fork failed\n");
        exit(EXIT_FAILURE);
      }

      else if (pid == 0) {    // if it's the child process
        break;
      }
    }

    if (active_child_proc == 0) {
      break;
    }

    if (pid != 0) {  // if it's the parent process
      pid = wait(&status);
      active_child_proc--;

      if (pid > 0) {  // the wait was successful
        if (WIFEXITED(status)) {
          printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        }
        else {
          printf("Child process %d didn't terminate normally\n", pid);
        }
      }
      else if (pid == -1) {
        fprintf(stderr, "Wait failed\n");
        exit(EXIT_FAILURE);
      }
      continue;
    }

    printf("File name: %s\n", dp->d_name);

    ThreadData* threads[max_threads];
    int fileExecuted = 1;
    void* threadStatus;

    while(fileExecuted) {

      if (createThreads(threads, max_threads, fileDescriptorIn, fileDescriptorOut) != 0) {
        fprintf(stderr, "Failed to create threads.\n");
        return 1;
      }
      
      for (int i = 0; i < max_threads; i++) {
        pthread_join(threads[i]->threadId, &threadStatus);
        while (threads[i]->wait_vector[i].first != NULL){
          WaitOrder* current = threads[i]->wait_vector[i].first->next;
          free(threads[i]->wait_vector[i].first);
          threads[i]->wait_vector[i].first = current;
        }

        if (i == max_threads - 1)
          free(threads[i]->wait_vector);
        free(threads[i]);
      }
      
      if ((int)(intptr_t)threadStatus != BARRIER_EXIT) {
        fileExecuted = 0; // exits the file when it was all read
        break;
      }
    }

    close(fileDescriptorIn);
    close(fileDescriptorOut);
    exit(EXIT_SUCCESS);
  }
  closedir(dirp);
  ems_terminate();
  return 0;
}

