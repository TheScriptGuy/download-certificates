#include "read_file.h"

FILE *open_input_file(const char *filename) {
    return fopen(filename, "r");
}

