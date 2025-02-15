#include "http_helper.h"
#include <string.h>

const char *get_mime_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0) return "text/html";
    if (strcasecmp(file_ext, "css") == 0) return "text/css";
    if (strcasecmp(file_ext, "js") == 0) return "application/javascript";
    if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(file_ext, "png") == 0) return "image/png";
    if (strcasecmp(file_ext, "gif") == 0) return "image/gif";
    if (strcasecmp(file_ext, "mp3") == 0) return "audio/mpeg";
    if (strcasecmp(file_ext, "mp4") == 0) return "video/mp4";
    if (strcasecmp(file_ext, "pdf") == 0) return "application/pdf";
    if (strcasecmp(file_ext, "txt") == 0) return "text/plain";
    if (strcasecmp(file_ext, "json") == 0) return "application/json";
    return "application/octet-stream";
}
