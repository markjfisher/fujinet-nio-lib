/**
 * @file tcp_stream.c
 * @brief FujiNet TCP Streaming Example - Non-blocking reads
 * 
 * Demonstrates non-blocking TCP reads for real-time applications.
 * This pattern is suitable for applications like games that need to
 * fetch frames of data without blocking timeouts.
 * 
 * Key concepts:
 *   - fn_read() returns FN_ERR_NOT_READY when no data is available
 *   - No application-level timeouts needed for real-time polling
 *   - Server responds immediately with available data or NotReady
 * 
 * Usage:
 *   Set environment variables to configure:
 *     FN_PORT       - Serial port device (default: /dev/ttyUSB0)
 *     FN_TEST_URL   - TCP URL (default: tcp://localhost:7777)
 * 
 * Build for Linux:
 *   make TARGET=linux
 * 
 * Build for Atari:
 *   make TARGET=atari
 */

/* Feature test macros for POSIX functions - must be before any includes */
#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <unistd.h>
#endif

#include "fujinet-nio.h"

/* Default configuration */
#ifndef FN_TCP_HOST
#define FN_TCP_HOST "localhost"
#endif

#ifndef FN_TCP_PORT
#define FN_TCP_PORT "7777"
#endif

/* Number of frame iterations to demonstrate */
#ifndef FN_FRAME_COUNT
#define FN_FRAME_COUNT 100
#endif

/* Maximum frame size */
#define MAX_FRAME_SIZE 256

/* Minimum bytes we need for a valid frame */
#define MIN_FRAME_SIZE 1

/**
 * Get current time in milliseconds (for timing stats)
 */
static unsigned long get_time_ms(void)
{
#ifdef __linux__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#else
    /* Fallback for other platforms - less precise */
    static unsigned long counter = 0;
    return counter++;
#endif
}

/**
 * Read a frame of data using non-blocking pattern.
 * 
 * This mimics the bounce-world-client pattern:
 * - Read up to max bytes
 * - Return immediately with what's available
 * - No timeouts on each read call
 * 
 * @param handle     Session handle
 * @param buf        Buffer to receive data
 * @param max_bytes  Maximum bytes to read
 * @param bytes_read Output: bytes actually read
 * @param eof        Output: true if peer closed connection
 * @return FN_OK on success, FN_ERR_NOT_READY if no data, error code on failure
 */
static uint8_t read_frame(fn_handle_t handle,
                          uint8_t *buf,
                          uint16_t max_bytes,
                          uint16_t *bytes_read,
                          uint8_t *eof)
{
    uint8_t result;
    uint8_t flags;
    uint16_t n;
    
    (void)bytes_read;
    (void)eof;
    
    *bytes_read = 0;
    *eof = 0;
    
    /* Try to read up to max_bytes */
    result = fn_read(handle, 0, buf, max_bytes, &n, &flags);
    
    if (result == FN_ERR_NOT_READY) {
        /* No data available yet - this is normal for non-blocking reads */
        return FN_ERR_NOT_READY;
    }
    
    if (result != FN_OK) {
        return result;
    }
    
    *bytes_read = n;
    
    /* Check for EOF (peer closed connection) */
    if (flags & FN_READ_EOF) {
        *eof = 1;
    }
    
    return FN_OK;
}

/**
 * Simple frame processor - just print the data
 */
static void process_frame(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    printf("Frame: %u bytes: ", len);
    for (i = 0; i < len && i < 32; i++) {
        if (data[i] >= 32 && data[i] < 127) {
            printf("%c", data[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

int main(void)
{
    uint8_t result;
    fn_handle_t handle;
    char url[256];
    uint8_t frame_buf[MAX_FRAME_SIZE];
    uint16_t bytes_read;
    uint8_t eof;
    unsigned long start_time, end_time;
    unsigned int frames_received = 0;
    unsigned int not_ready_count = 0;
    unsigned int total_bytes = 0;
    int i;
    
    printf("FujiNet-NIO TCP Streaming Example\n");
    printf("=================================\n\n");
    
    /* Initialize the library */
    printf("Initializing...\n");
    result = fn_init();
    if (result != FN_OK) {
        printf("Init failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    /* Check if device is ready */
    if (!fn_is_ready()) {
        printf("FujiNet device not ready!\n");
        return 1;
    }
    
    printf("Device ready.\n\n");
    
    /* Build URL from environment or defaults */
    const char *env_url = getenv("FN_TEST_URL");
    if (env_url) {
        strncpy(url, env_url, sizeof(url) - 1);
        url[sizeof(url) - 1] = '\0';
    } else {
        snprintf(url, sizeof(url), "tcp://%s:%s", FN_TCP_HOST, FN_TCP_PORT);
    }
    
    printf("Connecting to: %s\n", url);
    
    /* Open TCP connection */
    result = fn_open(&handle, 0, url, 0);
    if (result != FN_OK) {
        printf("Open failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    printf("Connected. Handle: %u\n\n", handle);
    
    /* Send initial request to trigger server responses */
    const char *request = "STREAM\n";
    uint16_t written;
    result = fn_write(handle, 0, (const uint8_t *)request, strlen(request), &written);
    if (result != FN_OK) {
        printf("Write failed: %s\n", fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    printf("Sent request: %s\n", request);
    
    printf("\nStarting frame loop (%d iterations)...\n", FN_FRAME_COUNT);
    printf("Each read is non-blocking - no timeouts!\n\n");
    
    start_time = get_time_ms();
    
    /* Main frame loop - demonstrate non-blocking reads */
    for (i = 0; i < FN_FRAME_COUNT; i++) {
        result = read_frame(handle, frame_buf, MAX_FRAME_SIZE, &bytes_read, &eof);
        
        if (result == FN_OK) {
            if (bytes_read > 0) {
                frames_received++;
                total_bytes += bytes_read;
                process_frame(frame_buf, bytes_read);
            }
            if (eof) {
                printf("Server closed connection.\n");
                break;
            }
        } else if (result == FN_ERR_NOT_READY) {
            /* No data available - this is expected for non-blocking reads */
            not_ready_count++;
            /* In a real app, you'd do other work here (render, input, etc.) */
            #ifdef __linux__
            {
                struct timespec ts = {0, 10000000}; /* 10ms */
                nanosleep(&ts, NULL);
            }
            #endif
        } else {
            printf("Read error: %s\n", fn_error_string(result));
            break;
        }
    }
    
    end_time = get_time_ms();
    
    /* Print statistics */
    printf("\n=== Statistics ===\n");
    printf("Frames received:  %u\n", frames_received);
    printf("Total bytes:      %u\n", total_bytes);
    printf("Not-ready count:  %u (normal for non-blocking)\n", not_ready_count);
    printf("Elapsed time:     %lu ms\n", end_time - start_time);
    if (frames_received > 0) {
        printf("Avg frame size:   %u bytes\n", total_bytes / frames_received);
    }
    
    /* Close connection */
    printf("\nClosing connection...\n");
    result = fn_close(handle);
    if (result != FN_OK) {
        printf("Close failed: %s\n", fn_error_string(result));
    }
    
    printf("Done.\n");
    return 0;
}
