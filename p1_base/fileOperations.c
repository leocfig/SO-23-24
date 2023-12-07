#include <dirent.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include "fileOperations.h"


// Dúvidas:
// - Podemos assumir que o tamanho da nova extensão é sempre menor do que a extensão antiga? (If not, temos de fazer um realloc)
// - LInha ssize_t bytes_written = write(fdOut, buffer + done, (size_t)len); no writeFile
// - Erros
// - Computadores dos labs
// - Pergunta 34
// - Aquilo de supormos que .jobs era menor que .out


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


char *change_extension(char *filename, const char *extension) { // checkar isto!!!

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


char *buffer_to_string(const unsigned int *buffer, size_t buffer_size, int function_called) {
  
  size_t size = (buffer_size * (UNS_INT_SIZE + 1) + 1);
  char *char_buffer = (char *)malloc(size * sizeof(char));

  if (char_buffer == NULL) {
    fprintf(stderr, "Memory allocation for buffer failed\n");
    return NULL;
  }

  size_t len = (size_t)snprintf(char_buffer, size, "%u", buffer[0]);

  if (len >= size) {
    fprintf(stderr, "Error in snprintf\n");
    free(char_buffer);
    return NULL;
  }

  if (function_called == SHOW_KEY) {
    for (size_t i = 1; i < buffer_size; i++) {
      len += (size_t)snprintf(char_buffer + len, size - len, " %u", buffer[i]);

      if (len >= size) {
        fprintf(stderr, "Error in snprintf\n");
        free(char_buffer);
        return NULL;
      }
    }
  }

  snprintf(char_buffer + len, size - len, "\n");
  return char_buffer;
}


int openFile(const char *path, int flags, mode_t mode) {

  int fd = open(path, flags, mode);
  if (fd < 0){
    fprintf(stderr, "open error: %s\n", strerror(errno));
    // return -1; aqui não fazemos nada né? Porque o stor tinha dito que podiamos fazer o que quiséssemos
  }
  return fd;
}


void write_inFile(int fdOut, const char *buffer) {

  ssize_t len = (ssize_t)strlen(buffer);
  ssize_t done = 0;
  
  while (len > 0) {
    ssize_t bytes_written = write(fdOut, buffer + done, (size_t)len);

    if (bytes_written < 0) {
      fprintf(stderr, "write error: %s\n", strerror(errno));
      break;
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


