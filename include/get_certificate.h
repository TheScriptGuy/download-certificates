#ifndef GET_CERTIFICATE_H
#define GET_CERTIFICATE_H

#include <stddef.h>
#include <stdbool.h>

int download_certificate(const char *hostname, const char *port, const char *output_dir,
                         char *result_message, size_t max_length, int timeout, bool overwrite, int worker_id);

#endif // GET_CERTIFICATE_H
