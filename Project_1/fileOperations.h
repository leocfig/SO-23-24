#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <sys/types.h>


#define UNS_INT_SIZE 10
#define SHOW_KEY     0
#define LIST_KEY     1
#define ERROR_MEMORY 25

extern pthread_mutex_t mutex_write;


int has_extension(const char *filename, const char *extension);

char *change_extension(char *filename, const char *extension);

char *buffer_to_string(const unsigned int *buffer, size_t buffer_size, int function_called);

int openFile(const char *path, int flags, mode_t mode);

void write_inFile(int fdOut, const char *buffer);


#endif // FILE_OPERATIONS_H