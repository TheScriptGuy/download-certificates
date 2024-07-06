#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdbool.h>

bool sha256sum(const unsigned char *data, size_t len, char *output, size_t output_size);
bool get_ssl_error(char *error_message, size_t max_length);

#endif // UTILS_H
