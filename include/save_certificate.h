#ifndef SAVE_CERTIFICATE_H
#define SAVE_CERTIFICATE_H

#include <openssl/x509.h>
#include <stdbool.h>

int save_certificate(X509 *cert, const char *output_dir, bool overwrite, int worker_id);

#endif // SAVE_CERTIFICATE_H
