#include <dirent.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include "fileOperations.h"


// Dúvidas:
// - Temos de printar o erro na linha de comandos com o write ou não é suposto printar mesmo nada na linha de comandos?
// - Podemos criar ficheiros novos? -> Então temos de mudar a makefile?
// - Podemos usar o errMsg??? Se sim, não tinhamos de fazer o write?
// - Quando um dos ficheiros não abre, é suposto além de printar o erro, fazer o quê?
// - Código repetido???


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


int openFile(const char *path, int flags) {

  int fd = open(path, flags);
  if (fd < 0){
    char* error_message = strcat(strerror(errno), "\n");
    char* error = "open error: ";
    strcat(error, error_message);
    size_t error_message_length = strlen(error);

    write(STDERR_FILENO, error, error_message_length);
    return -1;
  }
  return fd;
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


int *readDirectory(const char *dirpath) {

  DIR *dirp;
  struct dirent *dp;
  dirp = opendir(dirpath);

  if (dirp == NULL) {
    //errMsg("opendir failed on '%s'", dirpath);
    return NULL;
  }

  int* fileDescriptors = malloc((size_t) get_size_directory(dirpath) * sizeof(int));

  // Check if memory allocation is successful
  if (fileDescriptors == NULL) {
      write(STDERR_FILENO, "Error allocating memory\n", ERROR_MEMORY);
      return NULL;
  }

  for (;;) {
    errno = 0; /* To distinguish error from end-of-directory */
    dp = readdir(dirp);
    int i = 0;

    if (dp == NULL)
      break;
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0 || 
        !has_extension(dp->d_name, ".jobs"))
      continue; /* Skips . and .. and files with other extensions other than ".jobs" */

    fileDescriptors[i++] = openFile(dp->d_name, O_RDONLY);
  }
  return fileDescriptors;
}

