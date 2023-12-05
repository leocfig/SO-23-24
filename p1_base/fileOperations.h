#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H




#define ERROR_MEMORY 25


int has_extension(const char *filename, const char *extension);

char *change_extension(char *filename, const char *extension);

int openFile(const char *path, int flags);

void write_inFile(int fdOut, const char *buffer);

int get_size_directory(const char *dirpath);


#endif // FILE_OPERATIONS_H