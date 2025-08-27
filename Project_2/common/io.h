#ifndef COMMON_IO_H
#define COMMON_IO_H

#include <stddef.h>


/// Parses an unsigned integer from the given file descriptor.
/// @param fd The file descriptor to read from.
/// @param value Pointer to the variable to store the value in.
/// @param next Pointer to the variable to store the next character in.
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_uint(int fd, unsigned int *value, char *next);

/// Parses an integer from the given pipe.
/// @param pipe The pipe to read from.
/// @param value Pointer to the variable to store the value in.
/// @return 0 if the integer was read successfully, 1 otherwise.
int parse_int_pipe(int pipe, int *value);

/// Parses an unsigned integer from the given pipe.
/// @param pipe The pipe to read from.
/// @param value Pointer to the variable to store the value in.
/// @return 0 if the unsigned integer was read successfully, 1 otherwise.
int parse_uns_int_pipe(int pipe, unsigned int *value);

/// Parses a size_t from the given pipe.
/// @param pipe The pipe to read from.
/// @param value Pointer to the variable to store the value in.
/// @return 0 if the size_t was read successfully, 1 otherwise.
int parse_size_t_pipe(int pipe, size_t *value);

/// Parses an array of size_t from the given pipe.
/// @param pipe The pipe to read from.
/// @param value Pointer to the variable to store the value in.
/// @param num_elements The number of values to read.
/// @return 0 if the size_t array was read successfully, 1 otherwise.
int parse_size_t_array_pipe(int pipe, size_t *values, size_t num_elements);

/// Parses an array of unsigned integers from the given pipe.
/// @param pipe The pipe to read from.
/// @param value Pointer to the variable to store the value in.
/// @param num_elements The number of values to read.
/// @return 0 if the unsigned integer array was read successfully, 1 otherwise.
int parse_uns_int_array_pipe(int pipe, unsigned int *values, size_t num_elements);

/// Prints an integer to the given pipe.
/// @param pipe The pipe to write to.
/// @param value The value to write.
/// @return 0 if the integer was written successfully, 1 otherwise.
int print_int_pipe(int pipe, const int value);

/// Prints an unsigned integer to the given pipe.
/// @param pipe The pipe to write to.
/// @param value The value to write.
/// @return 0 if the unsigned integer was written successfully, 1 otherwise.
int print_uns_int_pipe(int pipe, const unsigned int value);

/// Prints a size_t to the given pipe.
/// @param pipe The pipe to write to.
/// @param value The value to write.
/// @return 0 if the size_t was written successfully, 1 otherwise.
int print_size_t_pipe(int pipe, const size_t value);

/// Prints an array of unsigned integers to the given pipe.
/// @param pipe The pipe to write to.
/// @param values The pointer to the values to write.
/// @param num_elements The size of the array to write.
/// @return 0 if the size_t array was written successfully, 1 otherwise.
int print_size_t_array_pipe(int pipe, const size_t *values, size_t num_elements);

/// Prints an array of unsigned integers to the given pipe.
/// @param pipe The pipe to write to.
/// @param values The pointer to the values to write.
/// @param num_elements The size of the array to write.
/// @return 0 if the unsigned integer array was written successfully, 1 otherwise.
int print_uns_int_array_pipe(int pipe, const unsigned int *values, size_t num_elements);

/// Parses a string from the given pipe.
/// @param pipe The pipe to read from.
/// @param str Pointer to the variable to store the value in.
/// @param size The size of the string to read.
/// @return 0 if the string was read successfully, 1 otherwise.
int parse_str_pipe(int pipe, char *str, unsigned int size);

/// Prints an unsigned integer to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param value The value to write.
/// @return 0 if the integer was written successfully, 1 otherwise.
int print_uint(int fd, unsigned int value);

/// Writes a string to the given file descriptor.
/// @param fd The file descriptor to write to.
/// @param str The string to write.
/// @return 0 if the string was written successfully, 1 otherwise.
int print_str(int fd, const char *str);

/// Writes a string to the given pipe.
/// @param pipe The pipe to write to.
/// @param str Pointer to the variable to write.
/// @param size The size of the string to write.
/// @return 0 if the string was written successfully, 1 otherwise.
int print_str_pipe(int pipe, const char *str, int size);

/// Writes to the given file descriptor the matrix of the seats' status of an event.
/// @param out_fd The file descriptor to write to.
/// @param num_rows The number of rows in the event.
/// @param num_cols The number of columns in the event.
/// @param seats The pointer to the register of the events seats.
/// @return 0 if the operation was successfully, 1 otherwise.
int print_output_show(int out_fd, size_t num_rows, size_t num_cols, unsigned int *seats);

/// Transforms a buffer of unsigned integers and formats the values to the corresponding
/// output expected by a the command that called the instance.
/// @param buffer The pointer to values.
/// @param buffer_size The number of elements in the buffer.
/// @param function_called The key to the command that called the function: SHOW or LIST_EVENTS
/// @return Pointer to the resulting string.
char *buffer_to_string(const unsigned int *buffer, size_t buffer_size, int function_called);

#endif  // COMMON_IO_H
