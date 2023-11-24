#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H




#define ERROR_MEMORY 25


int has_extension(const char *filename, const char *extension);

int openFile(const char *path, int flags);

int get_size_directory(const char *dirpath);

int *readDirectory(const char *dirpath);


#endif // FILE_OPERATIONS_H