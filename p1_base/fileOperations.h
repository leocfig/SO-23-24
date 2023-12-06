#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <sys/types.h>


#define UNS_INT_SIZE 10
#define SHOW_KEY     0
#define LIST_KEY     1
#define ERROR_MEMORY 25


void print_unsigned_int_array(unsigned int *arr, size_t size);

int has_extension(const char *filename, const char *extension);

char *change_extension(char *filename, const char *extension);

char *buffer_to_string(const unsigned int *buffer, size_t buffer_size, int function_called);

int openFile(const char *path, int flags, mode_t mode);

void write_inFile(int fdOut, const char *buffer);

int get_size_directory(const char *dirpath);


#endif // FILE_OPERATIONS_H