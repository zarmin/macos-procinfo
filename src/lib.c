#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

const char *human_readable_size(const uint64_t bytes) {
    static char buffer[32];
    const char *units[] = {"", "K", "M", "G", "T"};
    int unit_idx = 0;
    double size = bytes;

    while (size >= 1024 && unit_idx < 4) {
        size /= 1024;
        unit_idx++;
    }

    snprintf(buffer, sizeof(buffer), "%.1f%s", size, units[unit_idx]);
    return buffer;
}
