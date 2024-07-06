#define _POSIX_C_SOURCE 200809L

#include "get_certificate.h"
#include "save_certificate.h"
#include "utils.h"
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

/**
 * Wait for a BIO to become ready for reading or writing.
 *
 * @param bio The BIO to wait for
 * @param timeout The maximum time to wait in seconds
 * @param for_read If non-zero, wait for read, otherwise wait for write
 * @return >0 if the BIO is ready, 0 on timeout, <0 on error
 */
static int wait_for_bio(BIO *bio, int timeout, int for_read) {
    int fd = BIO_get_fd(bio, NULL);
    if (fd < 0) return -1;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    struct timeval tv = {
        .tv_sec = timeout,
        .tv_usec = 0
    };

    if (for_read) {
        return select(fd + 1, &fds, NULL, NULL, &tv);
    } else {
        return select(fd + 1, NULL, &fds, NULL, &tv);
    }
}

/**
 * Download a certificate from a given hostname and port.
 *
 * @param hostname The hostname to connect to
 * @param port The port to connect to
 * @param output_dir The directory to save the certificate
 * @param result_message Buffer to store the result message
 * @param max_length Maximum length of the result message
 * @param timeout Connection timeout in seconds
 * @return 0 on success, -1 on failure
 */
int download_certificate(const char *hostname, const char *port, const char *output_dir,
                         char *result_message, size_t max_length, int timeout, bool overwrite, int worker_id) {

    SSL_CTX *ctx = NULL;
    SSL *ssl = NULL;
    BIO *bio = NULL;
    X509 *cert = NULL;
    int ret = -1;

    // Create SSL context
    ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        snprintf(result_message, max_length, "Failed to create SSL context");
        goto cleanup;
    }

    // Disable certificate verification (Note: This is not recommended for production use)
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    // Create BIO and set up SSL connection
    bio = BIO_new_ssl_connect(ctx);
    if (!bio) {
        snprintf(result_message, max_length, "Failed to create BIO");
        goto cleanup;
    }

    BIO_get_ssl(bio, &ssl);
    if (!ssl) {
        snprintf(result_message, max_length, "Failed to get SSL from BIO");
        goto cleanup;
    }

    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    // Set up connection details
    char address[512];
    if (snprintf(address, sizeof(address), "%s:%s", hostname, port) >= (int)sizeof(address)) {
        snprintf(result_message, max_length, "Worker %d: Hostname and port too long", worker_id);
        goto cleanup;
    }

    if (BIO_set_conn_hostname(bio, address) != 1) {
        snprintf(result_message, max_length, "Worker %d: Failed to set connection hostname", worker_id);
        goto cleanup;
    }

    // Set the hostname for SNI
    if (SSL_set_tlsext_host_name(ssl, hostname) != 1) {
        snprintf(result_message, max_length, "Worker %d: Failed to set TLS extension hostname", worker_id);
        goto cleanup;
    }

    // Set non-blocking mode
    BIO_set_nbio(bio, 1);

    /*
    // Try connecting with timeout
    while ((ret = BIO_do_connect(bio)) <= 0) {
        if (!BIO_should_retry(bio)) {
            snprintf(result_message, max_length, "Error connecting to %s:%s", hostname, port);
            get_ssl_error(result_message, max_length);
            goto cleanup;
        }
        if (wait_for_bio(bio, timeout, 0) <= 0) {
            snprintf(result_message, max_length, "Connection timeout to %s:%s", hostname, port);
            goto cleanup;
        }
    }

    // Try handshake with timeout
    while ((ret = BIO_do_handshake(bio)) <= 0) {
        if (!BIO_should_retry(bio)) {
            snprintf(result_message, max_length, "Failed to perform SSL handshake");
            get_ssl_error(result_message, max_length);
            goto cleanup;
        }
        if (wait_for_bio(bio, timeout, 0) <= 0) {
            snprintf(result_message, max_length, "SSL handshake timeout");
            goto cleanup;
        }
    }

    */
    // Try connecting with timeout
    while ((ret = BIO_do_connect(bio)) <= 0) {
        if (!BIO_should_retry(bio)) {
            unsigned long error_code = ERR_get_error();
            char error_string[256];
            ERR_error_string_n(error_code, error_string, sizeof(error_string));

            // Check if it's likely a DNS resolution failure
            // I don't think these errors in error_string are valid
            snprintf(result_message, max_length, error_string);
            if (strstr(error_string, "unknown host") ||
                strstr(error_string, "Name or service not known")) {
                snprintf(result_message, max_length, "Worker %d: DNS resolution failure for %s:%s", worker_id, hostname, port);
            } else {
                snprintf(result_message, max_length, "Worker %d: Connection failed to %s:%s",
                         worker_id, hostname, port);
            }
            goto cleanup;
        }
        if (wait_for_bio(bio, timeout, 0) <= 0) {
            snprintf(result_message, max_length, "Worker %d: Connection timeout to %s:%s", worker_id, hostname, port);
            goto cleanup;
        }
    }

    // Retrieve the server certificate
    cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        snprintf(result_message, max_length, "Worker %d: Failed to get server certificate for %s:%s", worker_id, hostname, port);
        get_ssl_error(result_message, max_length);
        goto cleanup;
    }

    // Save the certificate, passing the overwrite flag
    if (save_certificate(cert, output_dir, overwrite, worker_id) != 0) {
        snprintf(result_message, max_length, "Worker %d: Failed to save certificate for %s:%s", worker_id, hostname, port);
        goto cleanup;
    }

    //snprintf(result_message, max_length, "Certificate downloaded and saved successfully");
    ret = 0;

cleanup:
    if (cert) X509_free(cert);
    if (bio) BIO_free_all(bio);
    if (ctx) SSL_CTX_free(ctx);

    return ret;
}
