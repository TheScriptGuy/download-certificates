#define _POSIX_C_SOURCE 200809L

#include "save_certificate.h"
#include "utils.h"
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MAX_PATH_LENGTH 1024
#define SHA256_HEX_LENGTH (SHA256_DIGEST_LENGTH * 2)

/**
 * Save an X509 certificate to a file in the specified output directory.
 * The filename is generated from the SHA256 hash of the certificate content.
 *
 * @param cert The X509 certificate to save
 * @param output_dir The directory where the certificate should be saved
 * @return 0 on success, -1 on failure
 */
int save_certificate(X509 *cert, const char *output_dir, bool overwrite, int worker_id) {
    BIO *mem = NULL;
    FILE *file = NULL;
    char sha256_output[SHA256_HEX_LENGTH + 5]; // +5 for ".pem" and null terminator
    char file_path[MAX_PATH_LENGTH];
    int result = -1;

    // Create a memory BIO and write the certificate to it
    mem = BIO_new(BIO_s_mem());
    if (!mem) {
        fprintf(stderr, "Worker %d: Failed to create memory BIO\n", worker_id);
        goto cleanup;
    }

    if (!PEM_write_bio_X509(mem, cert)) {
        fprintf(stderr, "Worker %d: Failed to write certificate to memory BIO\n", worker_id);
        goto cleanup;
    }

    // Get the certificate data from the memory BIO
    BUF_MEM *bptr;
    BIO_get_mem_ptr(mem, &bptr);

    // Calculate SHA256 hash of the certificate data
    if (!sha256sum((unsigned char *)bptr->data, bptr->length, sha256_output, sizeof(sha256_output))) {
        fprintf(stderr, "Worker %d: Failed to calculate SHA256 hash\n", worker_id);
        goto cleanup;
    } 
  
    // Append ".pem" to the hash to create the filename
    if (snprintf(sha256_output + SHA256_HEX_LENGTH, 5, ".pem") >= 5) {
        fprintf(stderr, "Worker %d: Buffer overflow when appending .pem\n", worker_id);
        goto cleanup;
    }

    // Construct the full file path
    if (snprintf(file_path, sizeof(file_path), "%s/%s", output_dir, sha256_output) >= (int)sizeof(file_path)) {
        fprintf(stderr, "Worker %d: File path too long\n", worker_id);
        goto cleanup;
    }

    // Modify the file opening logic
    const char *mode = overwrite ? "w" : "wx";
    file = fopen(file_path, mode);
    if (!file) {
        if (errno == EEXIST && !overwrite) {
            fprintf(stderr, "Worker %d: Certificate file already exists: %s\n", worker_id, file_path);
            result = 0; // Not treating this as an error when not overwriting
        } else {
            fprintf(stderr, "Worker %d: Failed to open file %s for writing: %s\n", worker_id, file_path, strerror(errno));
        }
        goto cleanup;
    }

    // Write the certificate data to the file
    if (fwrite(bptr->data, 1, bptr->length, file) != (size_t)bptr->length) {
        fprintf(stderr, "Worker %d: Failed to write certificate data to file\n", worker_id);
        goto cleanup;
    } else {
        fprintf(stderr, "Worker %d: Certificate downloaded and saved successfully: %s\n", worker_id, file_path);
    }

    result = 0; // Success

cleanup:
    if (mem) BIO_free(mem);
    if (file) fclose(file);
    return result;
}
