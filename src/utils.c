#include "utils.h"
#include <openssl/sha.h>
#include <openssl/err.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Compute the SHA256 hash of the input data and format it as a hexadecimal string.
 *
 * @param data Pointer to the input data buffer.
 * @param len Length of the input data in bytes.
 * @param output Buffer to store the resulting hexadecimal string (must be at least 65 bytes).
 * @return true if successful, false if an error occurred.
 */
bool sha256sum(const unsigned char *data, size_t len, char *output, size_t output_size) {
    if (data == NULL || output == NULL || output_size < (SHA256_DIGEST_LENGTH * 2 + 1)) {
        return false;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (SHA256(data, len, hash) == NULL) {
        return false;
    }

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        if (snprintf(output + (i * 2), 3, "%02x", hash[i]) != 2) {
            return false;
        }
    }
    output[SHA256_DIGEST_LENGTH * 2] = '\0';
    return true;
}

/**
 * @brief Retrieve and format OpenSSL error messages.
 *
 * @param error_message Buffer to store the formatted error message.
 * @param max_length Maximum length of the error_message buffer.
 * @return true if successful, false if an error occurred or buffer was too small.
 */

bool get_ssl_error(char *error_message, size_t max_length) {
    if (error_message == NULL || max_length == 0) {
        return false;
    }

    error_message[0] = '\0';
    unsigned long err;
    size_t current_length = 0;
    bool buffer_full = false;

    while ((err = ERR_get_error()) != 0 && !buffer_full) {
        char err_msg[256];
        ERR_error_string_n(err, err_msg, sizeof(err_msg));

        int written = snprintf(error_message + current_length, 
                               max_length - current_length, 
                               " error: %s", err_msg);

        if (written < 0 || (size_t)written >= max_length - current_length) {
            buffer_full = true;
        } else {
            current_length += (size_t)written;
        }
    }

    return !buffer_full;
}
