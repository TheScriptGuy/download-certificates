#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <errno.h>
#include <limits.h>
#include <time.h>  // For nanosleep
#include <stdbool.h>
#include "read_file.h"
#include "get_certificate.h"
#include "save_certificate.h"
#include "utils.h"

#define DEFAULT_PORT "443"
#define DEFAULT_WORKERS 1
#define DEFAULT_TIMEOUT 3
#define MAX_WORKERS 100
#define MAX_LINE_LENGTH 256
#define MAX_RESULT_LENGTH 2048

// Mutex for synchronizing access to the input file
static pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex for synchronizing print operations
static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
// Global file pointer for the input file
static FILE *input_file = NULL;

// Structure to hold worker thread data
typedef struct {
    int worker_id;
    const char *output_dir;
    double delay;
    int timeout;
    bool overwrite;
} worker_data_t;

/**
 * Worker thread function.
 * Reads hostnames from the input file, downloads certificates, and prints results.
 * 
 * @param arg Pointer to worker_data_t structure
 * @return NULL
 */
static void *worker_thread(void *arg) {
    worker_data_t *data = (worker_data_t *)arg;
    char line[MAX_LINE_LENGTH];
    char result_message[MAX_RESULT_LENGTH];

    while (1) {
        // Lock the file mutex before reading from the input file
        if (pthread_mutex_lock(&file_mutex) != 0) {
            fprintf(stderr, "Failed to lock file mutex\n");
            return NULL;
        }

        // Read a line from the input file
        if (fgets(line, sizeof(line), input_file) == NULL) {
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        // Unlock the file mutex after reading
        if (pthread_mutex_unlock(&file_mutex) != 0) {
            fprintf(stderr, "Failed to unlock file mutex\n");
            return NULL;
        }

        // Parse the hostname and port from the input line
        char *hostname = strtok(line, ":\n");
        char *port = strtok(NULL, "\n");

        if (port == NULL) {
            port = DEFAULT_PORT;
        }

        // Format the result message
        int ret = snprintf(result_message, sizeof(result_message), 
                           "Worker %d: Attempting to connect to %s:%s...", 
                           data->worker_id, hostname, port);
        if (ret < 0 || (size_t)ret >= sizeof(result_message)) {
            fprintf(stderr, "Error formatting result message\n");
            continue;
        }
        
        // Download the certificate
        download_certificate(hostname, port, data->output_dir, result_message,
                     sizeof(result_message), data->timeout, data->overwrite, data->worker_id);

        // Lock the print mutex before printing
        if (pthread_mutex_lock(&print_mutex) != 0) {
            fprintf(stderr, "Failed to lock print mutex\n");
            return NULL;
        }

        // Print the result
        printf("%s\n", result_message);

        // Unlock the print mutex after printing
        if (pthread_mutex_unlock(&print_mutex) != 0) {
            fprintf(stderr, "Failed to unlock print mutex\n");
            return NULL;
        }

        // Delay if specified
        if (data->delay > 0) {
            struct timespec ts;
            ts.tv_sec = (time_t)data->delay;
            ts.tv_nsec = (long)((data->delay - ts.tv_sec) * 1e9);
            nanosleep(&ts, NULL);
        }
    }

    // Print completion message
    if (pthread_mutex_lock(&print_mutex) != 0) {
        fprintf(stderr, "Failed to lock print mutex\n");
        return NULL;
    }

    printf("Worker %d: finished.\n", data->worker_id);

    if (pthread_mutex_unlock(&print_mutex) != 0) {
        fprintf(stderr, "Failed to unlock print mutex\n");
        return NULL;
    }

    return NULL;
}

/**
 * Prints usage information for the program.
 * 
 * @param program_name The name of the program
 */
static void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s -if <input_file> -od <output_directory> [-delay <seconds>] [-workers <number>] [-overwrite]\n", program_name);
    fprintf(stderr, "  -if         input file of hostnames and ports to connect to.\n");
    fprintf(stderr, "  -od         the directory where you want to save all the downloaded certificates.\n");
    fprintf(stderr, "  -delay      the delay between each worker's request. Default is 0.\n");
    fprintf(stderr, "  -workers    the number of workers making requests to websites. Default is 1.\n");
    fprintf(stderr, "  -timeout    the time in seconds to wait before assuming the connection is not responding. Default is 3.\n");
    fprintf(stderr, "  -overwrite  allow overwriting of existing certificate files.\n");
}

int main(int argc, char *argv[]) {
    const char *input_filename = NULL;
    const char *output_dir = NULL;
    double delay = 0;
    int workers = DEFAULT_WORKERS;
    int timeout = DEFAULT_TIMEOUT;
    bool overwrite = false;

    // Parse command line arguments
    if (argc < 5) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-if") == 0 && i + 1 < argc) {
            input_filename = argv[++i];
        } else if (strcmp(argv[i], "-od") == 0 && i + 1 < argc) {
            output_dir = argv[++i];
        } else if (strcmp(argv[i], "-delay") == 0 && i + 1 < argc) {
            char *endptr;
            delay = strtod(argv[++i], &endptr);
            if (*endptr != '\0' || delay < 0) {
                fprintf(stderr, "Invalid delay value\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-workers") == 0 && i + 1 < argc) {
            char *endptr;
            long workers_long = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0' || workers_long <= 0 || workers_long > MAX_WORKERS) {
                fprintf(stderr, "Invalid number of workers. Must be between 1 and %d.\n", MAX_WORKERS);
                return EXIT_FAILURE;
            }
            workers = (int)workers_long;
        } else if (strcmp(argv[i], "-timeout") == 0 && i + 1 < argc) {
            char *endptr;
            long timeout_long = strtol(argv[++i], &endptr, 10);
            if (*endptr != '\0' || timeout_long <= 0 || timeout_long > INT_MAX) {
                fprintf(stderr, "Invalid timeout value\n");
                return EXIT_FAILURE;
            }
            timeout = (int)timeout_long;
        } else if (strcmp(argv[i], "-overwrite") == 0) {
            overwrite = true;
        } else {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Check if required arguments are provided
    if (!input_filename || !output_dir) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Create output directory if it doesn't exist
    struct stat st = {0};
    if (stat(output_dir, &st) == -1) {
        if (mkdir(output_dir, 0700) != 0) {
            perror("Failed to create output directory");
            return EXIT_FAILURE;
        }
    }

    // Initialize OpenSSL
    if (OPENSSL_init_ssl(0, NULL) == 0) {
        fprintf(stderr, "Failed to initialize OpenSSL\n");
        return EXIT_FAILURE;
    }

    // Open input file
    input_file = open_input_file(input_filename);
    if (!input_file) {
        perror("Failed to open input file");
        return EXIT_FAILURE;
    }

    // Create and start worker threads
    pthread_t threads[MAX_WORKERS];
    worker_data_t data[MAX_WORKERS];

    for (int i = 0; i < workers; i++) {
        data[i].worker_id = i + 1;
        data[i].output_dir = output_dir;
        data[i].delay = delay;
        data[i].timeout = timeout;
        data[i].overwrite = overwrite;
        if (pthread_create(&threads[i], NULL, worker_thread, &data[i]) != 0) {
            perror("Failed to create thread");
            fclose(input_file);
            return EXIT_FAILURE;
        }
    }

    // Wait for all threads to complete
    for (int i = 0; i < workers; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
        }
    }

    // Clean up
    fclose(input_file);
    OPENSSL_cleanup();

    return EXIT_SUCCESS;
}
