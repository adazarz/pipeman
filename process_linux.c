#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "process.h"

void get_yaml(char *buffer, size_t size) {
    ssize_t n = readlink("/proc/self/exe", buffer, size - 1);
    if (n >= 0) {
        buffer[n] = '\0';
    }
    char *last_slash = strrchr(buffer, '/');
    snprintf(last_slash+1, 13, "reports.yaml");
}