#include "file_helper.h"
#include <fcntl.h>
#include <string.h>
#include <windows.h>

int get_file_descriptor(const char *directory, const char *file_name) {
    char src_file[256];
    snprintf(src_file, sizeof(src_file), "%s%s", directory, file_name);
    return open(src_file, O_RDONLY | O_BINARY);
}

const char *get_file_extension(const char *file_name) {
    char *ext = strrchr(file_name, '.');
    return (ext != NULL) ? ext + 1 : "";
}
