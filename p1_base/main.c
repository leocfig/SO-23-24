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
#include <pthread.h>

#include "fileOperations.h"
#include "operations.h"
#include "constants.h"
#include "parser.h"


typedef struct {
  pthread_t threadId;
  int fileDescriptorIn;
  int fileDescriptorOut;
  int vector_position;
} ThreadData;

pthread_mutex_t mutex_1;
pthread_mutex_t mutex_2;
//pthread_rwlock_t rwl;


void* processCommand(void* arg) {
  unsigned int event_id, delay; 
  //thread_id;
  size_t num_rows, num_columns, num_coords;
  size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

  ThreadData* threadData = (ThreadData*)arg;

  //pthread_t threadId = threadData->threadId;
  int fileDescriptorIn = threadData->fileDescriptorIn;
  int fileDescriptorOut = threadData->fileDescriptorOut;

  printf("fdIn: %d\n", fileDescriptorIn);
  printf("fdOut: %d\n", fileDescriptorOut);

  int endOfFile = 1;

  while (endOfFile) {

    pthread_mutex_lock(&mutex_1);
    enum Command command = get_next(fileDescriptorIn);
    pthread_mutex_unlock(&mutex_1);
    
    printf("Command: %d\n", command);

    switch (command) {
      
      case CMD_CREATE:
        if (parse_create(fileDescriptorIn, &event_id, &num_rows, &num_columns) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_create(event_id, num_rows, num_columns)) {
          fprintf(stderr, "Failed to create event\n");
        }

        break;

      case CMD_RESERVE:
        num_coords = parse_reserve(fileDescriptorIn, MAX_RESERVATION_SIZE, &event_id, xs, ys);

        if (num_coords == 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_reserve(event_id, num_coords, xs, ys, mutex_2)) {
          fprintf(stderr, "Failed to reserve seats\n");
        }

        break;

      case CMD_SHOW:
        if (parse_show(fileDescriptorIn, &event_id) != 0) {
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (ems_show(fileDescriptorOut, event_id)) {
          fprintf(stderr, "Failed to show event\n");
        }
        break;

      case CMD_LIST_EVENTS:
        if (ems_list_events(fileDescriptorOut)) {
          fprintf(stderr, "Failed to list events\n");
        }

        break;

      case CMD_WAIT:
        if (parse_wait(fileDescriptorIn, &delay, NULL) == -1) {  // thread_id is not implemented
          fprintf(stderr, "Invalid command. See HELP for usage\n");
          continue;
        }

        if (delay > 0) {
          printf("Waiting...\n");
          ems_wait(delay);
        }

        break;


      case CMD_INVALID:
        fprintf(stderr, "Invalid command. See HELP for usage\n");
        break;

      case CMD_HELP:
        printf(
            "Available commands:\n"
            "  CREATE <event_id> <num_rows> <num_columns>\n"
            "  RESERVE <event_id> [(<x1>,<y1>) (<x2>,<y2>) ...]\n"
            "  SHOW <event_id>\n"
            "  LIST\n"
            "  WAIT <delay_ms> [thread_id]\n"  // thread_id is not implemented
            "  BARRIER\n"                      // Not implemented
            "  HELP\n");

        break;

      case CMD_BARRIER:  // Not implemented
      case CMD_EMPTY:
        break;
      case EOC:
        endOfFile=0;
      break;
    }
  }
  pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (pthread_mutex_init(&mutex_1, NULL) != 0) {
    fprintf(stderr, "Failed to initialize the mutex.\n");
    return 1;
  }

  if (pthread_mutex_init(&mutex_2, NULL) != 0) {
    fprintf(stderr, "Failed to initialize the mutex.\n");
    return 1;
  }

  //if (pthread_rwlock_init(&rwl, NULL) != 0) {
    //fprintf(stderr, "Error: Failed to initialize the read-write lock.\n");
    //return 1;
  //}

  if (argc > 4) {
    char *endptr;
    unsigned long int delay = strtoul(argv[4], &endptr, 10);

    if (*endptr != '\0' || delay > UINT_MAX) {
      fprintf(stderr, "Invalid delay value or value too large\n");
      return 1;
    }

    state_access_delay_ms = (unsigned int)delay;
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

  size_t sizePath = strlen(dirpath);
  pid_t pid;
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
      fileDescriptorOut = openFile(change_extension(filePath, "out"), 
                              O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);

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

    if (threads == NULL) {
      fprintf(stderr, "Failed to allocate memory for threads\n");
      return 1;
    }

    for (int i = 0; i < max_threads; i++) {

      ThreadData* threadData = (ThreadData*)malloc(sizeof(ThreadData));
      if (threadData == NULL) {
          fprintf(stderr, "Failed to allocate memory for ThreadData\n");
          return 1;
      }

      threads[i] = threadData;
      threadData->vector_position = i + 1;
      threadData->fileDescriptorIn = fileDescriptorIn;
      threadData->fileDescriptorOut = fileDescriptorOut;
      pthread_create(&threads[i]->threadId, NULL, processCommand, (void*)threadData);
    }

    for (int i = 0; i < max_threads; i++) {
      pthread_join(threads[i]->threadId, NULL);
      free(threads[i]);
    }
    close(fileDescriptorIn);
    close(fileDescriptorOut);
    exit(EXIT_SUCCESS);
  }
  closedir(dirp);
  pthread_mutex_destroy(&mutex_1);
  pthread_mutex_destroy(&mutex_2);
  //pthread_rwlock_destroy(&rwl);
  ems_terminate();
  return 0;
}

