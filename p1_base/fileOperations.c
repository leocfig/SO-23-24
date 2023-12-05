#include <dirent.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "fileOperations.h"


// Dúvidas:
// - Podemos assumir que o tamanho da nova extensão é sempre menor do que a extensão antiga? (If not, temos de fazer um realloc)


int has_extension(const char *filename, const char *extension) {

  size_t filename_length = strlen(filename);
  size_t extension_length = strlen(extension);

  // Checks if the filename is at least as long as the extension
  if (filename_length >= extension_length) {
      // Compares the last characters of the filename with the extension
      return (strcmp(filename + filename_length - extension_length, extension) == 0);
  }

  return 0;  // File does not have the specific extension
}


char *change_extension(char *filename, const char *extension) {

  char *lastDot = strrchr(filename, '.');
  long int index = lastDot - filename + 1; // to start after the dot
  int extension_size = (int) strlen(extension);
  int i,j;

  for (i = 0,j = (int) index; i < extension_size; i++, j++) {
    filename[j] = extension[i];
  }
  filename[j] = '\0';
  return filename;
}


int openFile(const char *path, int flags) {

  int fd = open(path, flags);
  if (fd < 0){
    char* e = strerror(errno);
    strcat(e, "\n");
    char* error = "open error: ";
    strcat(error, e);
    size_t error_message_length = strlen(error);

    write(STDERR_FILENO, error, error_message_length);
    return -1;
  }
  return fd;
}


void write_inFile(int fdOut, const char *buffer) {

  int len = strlen(buffer);
  
  int done = 0;
  while (len > 0) {
    int bytes_written = write(fdOut, buffer + done, len);

    if (bytes_written < 0){
      fprintf(stderr, "write error: %s\n", strerror(errno));
    }

    /* might not have managed to write all, len becomes what remains */
    len -= bytes_written;
    done += bytes_written;
  }
}


int get_size_directory(const char *dirpath) {

  DIR *dirp;
  struct dirent *dp;
  dirp = opendir(dirpath);

  if (dirp == NULL) {
    //errMsg("opendir failed on '%s'", dirpath);
    return -1;
  }
  
  int count = 0;

  for (;;) {
    errno = 0; /* To distinguish error from end-of-directory */
    dp = readdir(dirp);
    if (dp == NULL)
      break;
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0 || 
        !has_extension(dp->d_name, ".jobs"))
      continue; /* Skips . and .. and files with other extensions other than ".job" */
    count++;
  }
  return count;
}


