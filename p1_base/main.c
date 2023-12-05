#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <fcntl.h>
#include <string.h>

#include "fileOperations.h"
#include "operations.h"
#include "constants.h"
#include "parser.h"


int main(int argc, char *argv[]) {
  unsigned int state_access_delay_ms = STATE_ACCESS_DELAY_MS;

  if (argc > 2) {
    char *endptr;
    unsigned long int delay = strtoul(argv[2], &endptr, 10);

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
  printf("%s\n", dirpath);
  fflush(stdout);
  //int *fileDescriptors = readDirectory(dirpath);
  //int size_directory = get_size_directory(dirpath);
  DIR *dirp;
  struct dirent *dp;
  dirp = opendir(dirpath);

  if (dirp == NULL){
    fprintf(stderr, "opendir failed on: %s\n", dirpath);
    return 1;
  }

  char *auxDirpath = argv[1];
  for (;;) {
    errno = 0; /* To distinguish error from end-of-directory */
    dp = readdir(dirp);



    // ALOCAR UM BUFFER PARA OS NOMES DOS FICHEIROS 
    // NO FIM DAR FREE DO NOME 

    if (dp == NULL)
      break;
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0 || 
        !has_extension(dp->d_name, ".jobs"))
      continue; /* Skips . and .. and files with other extensions other than ".jobs" */

    int fileDescriptor = openFile(strcat(auxDirpath, dp->d_name), O_RDONLY);
    int endOfFile =1;

    while (endOfFile) {
      unsigned int event_id, delay;
      size_t num_rows, num_columns, num_coords;
      size_t xs[MAX_RESERVATION_SIZE], ys[MAX_RESERVATION_SIZE];

      switch (get_next(fileDescriptor)) {
        case CMD_CREATE:
          if (parse_create(fileDescriptor, &event_id, &num_rows, &num_columns) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (ems_create(event_id, num_rows, num_columns)) {
            fprintf(stderr, "Failed to create event\n");
          }

          break;

        case CMD_RESERVE:
          num_coords = parse_reserve(fileDescriptor, MAX_RESERVATION_SIZE, &event_id, xs, ys);

          if (num_coords == 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (ems_reserve(event_id, num_coords, xs, ys)) {
            fprintf(stderr, "Failed to reserve seats\n");
          }

          break;

        case CMD_SHOW:
          if (parse_show(fileDescriptor, &event_id) != 0) {
            fprintf(stderr, "Invalid command. See HELP for usage\n");
            continue;
          }

          if (ems_show(event_id)) {
            fprintf(stderr, "Failed to show event\n");
          }
          fprintf(stdout, "fim show\n");
          fflush(stdout);
          break;

        case CMD_LIST_EVENTS:
          if (ems_list_events()) {
            fprintf(stderr, "Failed to list events\n");
          }

          break;

        case CMD_WAIT:
          if (parse_wait(fileDescriptor, &delay, NULL) == -1) {  // thread_id is not implemented
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
          close(fileDescriptor);
          endOfFile=0;
          break;
      }
    }
    fprintf(stdout, "fim file\n");
    fflush(stdout);
    close(fileDescriptor);
  }
  closedir(dirp);
  ems_terminate();
  return 0;
}
