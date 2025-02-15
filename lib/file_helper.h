#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <stdio.h>

int get_file_descriptor(const char *directory, const char *file_name);
const char *get_file_extension(const char *file_name);

#endif
