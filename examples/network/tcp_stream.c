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
 * Configuration via environment variables (all platforms):
 *   FN_TEST_URL   - Full URL (e.g., tcp://host:port)
 *   FN_TCP_HOST   - Host to connect to (default: "localhost")
 *   FN_TCP_PORT   - Port to connect to (default: "7777")
 *   FN_PORT       - Serial port device (default: /dev/ttyUSB0)
 * 
 * For cc65 targets (Atari, Apple, etc.), environment variables are populated
 * from compile-time defines since there's no shell environment:
 *   FN_TCP_HOST   - Compile with -DFN_TCP_HOST=\"host\"
 *   FN_TCP_PORT   - Compile with -DFN_TCP_PORT=\"port\"
 * 
 * Build for Linux:
 *   make TARGET=linux
 * 
 * Build for Atari:
 *   make TARGET=atari
 */

/* Feature test macros for POSIX functions - must be before any includes */
#ifndef __ATARI__
#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __ATARI__
#include <time.h>
#include <unistd.h>
#endif

#include "fujinet-nio.h"

/* ============================================================================
 * Compile-time Configuration (can be overridden via CFLAGS)
 * ============================================================================ */

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

/* URL buffer for building URLs */
#define URL_MAX_LEN 256
static char g_url[URL_MAX_LEN];

/* ============================================================================
 * cc65 Environment Setup
 * ============================================================================ */

#ifdef __CC65__

/* Static storage for environment strings (putenv doesn't copy!) */
static char env_fn_tcp_host[] = "FN_TCP_HOST=" FN_TCP_HOST;
static char env_fn_tcp_port[] = "FN_TCP_PORT=" FN_TCP_PORT;

/**
 * @brief Set up environment variables from compile-time defines for cc65.
 */
static void setup_env(void)
{
    putenv(env_fn_tcp_host);
    putenv(env_fn_tcp_port);
}

#endif /* __CC65__ */

/**
 * Get current time in milliseconds (for timing stats)
 */
static unsigned long get_time_ms(void)
{
#ifndef __ATARI__
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
 * @param handle       Session handle
 * @param offset       Read offset (cumulative bytes read so far)
 * @param buf          Buffer to receive data
 * @param max_bytes    Maximum bytes to read
 * @param bytes_read   Output: bytes actually read
 * @param eof          Output: true if peer closed connection
 * @return FN_OK on success, FN_ERR_NOT_READY if no data, error code on failure
 */
static uint8_t read_frame(fn_handle_t handle,
                          uint32_t offset,
                          uint8_t *buf,
                          uint16_t max_bytes,
                          uint16_t *bytes_read,
                          uint8_t *eof)
{
    uint8_t result;
    uint8_t flags;
    uint16_t n;
    
    *bytes_read = 0;
    *eof = 0;
    
    /* Try to read up to max_bytes at the current offset */
    result = fn_read(handle, offset, buf, max_bytes, &n, &flags);
    
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
    const char *url;
    uint8_t frame_buf[MAX_FRAME_SIZE];
    uint16_t bytes_read;
    uint8_t eof;
    unsigned long start_time, end_time;
    unsigned int frames_received = 0;
    unsigned int not_ready_count = 0;
    unsigned int total_bytes = 0;
    int i;
    
#ifdef __CC65__
    /* Set up environment from compile-time defines for cc65 */
    setup_env();
#endif
    
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
    url = getenv("FN_TEST_URL");
    if (url != NULL && url[0] != '\0') {
        /* Use the URL directly from environment */
    } else {
        const char *host = getenv("FN_TCP_HOST");
        const char *port = getenv("FN_TCP_PORT");
        
        if (host == NULL || host[0] == '\0') {
            host = FN_TCP_HOST;
        }
        if (port == NULL || port[0] == '\0') {
            port = FN_TCP_PORT;
        }
        
        snprintf(g_url, URL_MAX_LEN, "tcp://%s:%s", host, port);
        url = g_url;
    }
    
    printf("Connecting to: %s\n", url);
    
    /* Open TCP connection */
    result = fn_open(&handle, 0, url, 0);
    if (result != FN_OK) {
        printf("Open failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    printf("Connected. Handle: %u\n\n", handle);
    
    /* Note: For streaming servers, data arrives automatically.
     * For echo servers, you would send a request first to trigger responses.
     * This example assumes a streaming server that pushes data continuously.
     */
    
    printf("Starting frame loop (%d iterations)...\n", FN_FRAME_COUNT);
    printf("Each read is non-blocking - no timeouts!\n\n");
    
    start_time = get_time_ms();
    
    /* Main frame loop - demonstrate non-blocking reads */
    for (i = 0; i < FN_FRAME_COUNT; i++) {
        result = read_frame(handle, total_bytes, frame_buf, MAX_FRAME_SIZE, &bytes_read, &eof);
        
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
#ifndef __ATARI__
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
