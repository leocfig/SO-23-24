#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <sys/types.h>


#define UNS_INT_SIZE 10
#define ERROR_MEMORY 25


int has_extension(const char *filename, const char *extension);

char *change_extension(char *filename, const char *extension);

char *buffer_to_string(const unsigned int *buffer, size_t buffer_size);

int openFile(const char *path, int flags, mode_t mode);

void write_inFile(int fdOut, const char *buffer);

int get_size_directory(const char *dirpath);


#endif // FILE_OPERATIONS_H