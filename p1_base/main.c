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

#include "fileOperations.h"
#include "operations.h"
#include "constants.h"
#include "parser.h"


int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 3) {
    char *endptr;
    unsigned long int delay = strtoul(argv[3], &endptr, 10);

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
  //int max_proc = argv[2];
  
  DIR *dirp;
  struct dirent *dp;
  dirp = opendir(dirpath);

  if (dirp == NULL){
    fprintf(stderr, "opendir failed on: %s\n", dirpath);
    return 1;
  }

  size_t sizePath = strlen(dirpath);
  
  for (;;) {
    errno = 0; /* To distinguish error from end-of-directory */
    dp = readdir(dirp);
    
    if (dp == NULL)
          break;

    // +2 for ...
    char *filePath = (char *)malloc((sizePath + strlen(dp->d_name) + 2)* sizeof(char));

    if (filePath == NULL) {
      fprintf(stderr, "Memory allocation for filePath failed\n");
      return 1;
    }

    strcpy(filePath, dirpath);
    strcat(filePath, "/");
    
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0 || 
        !has_extension(dp->d_name, ".jobs")) {
      free(filePath);
      continue; /* Skips . and .. and files with other extensions other than ".jobs" */
    }  

    int fileDescriptorIn = open(strcat(filePath, dp->d_name), O_RDONLY, NULL);
    int fileDescriptorOut = openFile(change_extension(filePath, "out"), 
                            O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    
    free(filePath);

    int endOfFile = 1;

    while (endOfFile) {
      unsigned int event_id, delay;
      size_t num_rows, num_columns, num_coords;
      size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

      switch (get_next(fileDescriptorIn)) {
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

          if (ems_reserve(event_id, num_coords, xs, ys)) {
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
          close(fileDescriptorIn);
          close(fileDescriptorOut);
          endOfFile=0;
          break;
      }
    }
  }
  closedir(dirp);
  ems_terminate();
  return 0;
}
