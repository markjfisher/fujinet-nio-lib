/**
 * @file tcp_get.c
 * @brief Simple TCP/TLS Client Example
 * 
 * Demonstrates how to use the fujinet-nio-lib to perform TCP and TLS connections.
 * This example connects to a TCP/TLS server, sends a request, and reads the response.
 * 
 * Usage:
 *   Set environment variables to configure (Linux only):
 *     FN_TCP_HOST - Host to connect to (default: localhost)
 *     FN_TCP_PORT - Port to connect to (default: 7777)
 *     FN_TCP_TLS  - Set to "1" to enable TLS (default: disabled)
 *     FN_PORT     - Serial port device (default: /dev/ttyUSB0)
 * 
 *   Or use a URL:
 *     FN_TEST_URL - Full URL (e.g., tcp://host:port or tls://host:port?testca=1)
 * 
 * Build for Linux:
 *   make TARGET=linux tcp_get
 * 
 * Build for Atari:
 *   make TARGET=atari tcp_get
 * 
 * Examples:
 *   # Connect to local TCP echo server
 *   ./bin/linux/tcp_get
 * 
 *   # Connect to TLS server
 *   FN_TCP_HOST=127.0.0.1 FN_TCP_PORT=7778 FN_TCP_TLS=1 ./bin/linux/tcp_get
 * 
 *   # Using full URL
 *   FN_TEST_URL="tls://echo.fujinet.online:6001?testca=1" ./bin/linux/tcp_get
 */

/* Feature test macros for POSIX functions */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "fujinet-nio.h"

/* Default configuration - can be overridden via build defines */
#ifndef FN_DEFAULT_TCP_HOST
#define FN_DEFAULT_TCP_HOST "localhost"
#endif

#ifndef FN_DEFAULT_TCP_PORT
#define FN_DEFAULT_TCP_PORT 7777
#endif

/* Buffer for reading data - static for cc65 compatibility */
#define BUFFER_SIZE 512
static uint8_t buffer[BUFFER_SIZE];

/* URL buffer - static for cc65 compatibility */
#define URL_MAX_LEN 256
static char url_buffer[URL_MAX_LEN];

/* Idle timeout in seconds - stop reading after this long with no data */
#ifndef FN_IDLE_TIMEOUT_SECS
#define FN_IDLE_TIMEOUT_SECS 2
#endif

/* Simple request to send (echo test) */
static const char *default_request = "Hello from FujiNet-NIO!\r\n";

/**
 * @brief Get current time in seconds
 * 
 * For Linux: uses clock_gettime(CLOCK_MONOTONIC)
 * For Atari: returns 0 (no idle timeout on Atari)
 */
static double get_time_secs(void)
{
#ifndef __ATARI__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
#else
    return 0.0;
#endif
}

int main(void)
{
    uint8_t result;
    fn_handle_t handle;
    uint16_t bytes_read;
    uint16_t bytes_written;
    uint8_t flags;
    uint32_t total_read;
    uint16_t total_written;
    const char *url;
    const char *request;
    uint16_t request_len;
    
    printf("FujiNet-NIO TCP/TLS Client Example\n");
    printf("==================================\n\n");
    
    /* Get configuration from environment or use defaults */
#ifdef __ATARI__
    /* On Atari, use compile-time defaults */
    snprintf(url_buffer, URL_MAX_LEN, "tcp://%s:%u", FN_DEFAULT_TCP_HOST, FN_DEFAULT_TCP_PORT);
    url = url_buffer;
    request = default_request;
#else
    const char *host;
    const char *port_str;
    const char *tls_str;
    uint16_t port;
    uint8_t use_tls;
    
    /* Check for full URL first */
    url = getenv("FN_TEST_URL");
    if (url != NULL && url[0] != '\0') {
        /* Using full URL - nothing else needed */
    } else {
        /* Build URL from individual components */
        host = getenv("FN_TCP_HOST");
        if (host == NULL || host[0] == '\0') {
            host = FN_DEFAULT_TCP_HOST;
        }
        
        port_str = getenv("FN_TCP_PORT");
        if (port_str != NULL && port_str[0] != '\0') {
            port = (uint16_t)atoi(port_str);
        } else {
            port = FN_DEFAULT_TCP_PORT;
        }
        
        tls_str = getenv("FN_TCP_TLS");
        use_tls = (tls_str != NULL && tls_str[0] == '1');
        
        /* Build URL */
        if (use_tls) {
            snprintf(url_buffer, URL_MAX_LEN, "tls://%s:%u?testca=1", host, port);
        } else {
            snprintf(url_buffer, URL_MAX_LEN, "tcp://%s:%u", host, port);
        }
        url = url_buffer;
    }
    
    /* Get custom request from environment, or use default */
    request = getenv("FN_TCP_REQUEST");
    if (request == NULL || request[0] == '\0') {
        request = default_request;
    }
#endif
    
    printf("URL: %s\n", url);
    printf("\n");
    
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
    
    /* Open connection - URL scheme determines protocol:
     * tcp://host:port - plain TCP
     * tls://host:port?testca=1 - TLS with test CA
     * tls://host:port?insecure=1 - TLS with cert verification disabled
     */
    printf("Opening connection...\n");
    result = fn_open(&handle, 0, url, 0);
    
    if (result != FN_OK) {
        printf("Connection failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    printf("Handle: %u\n", handle);
    printf("Connection established.\n");
    
    /* Send request */
    request_len = (uint16_t)strlen(request);
    printf("\nSending data (%u bytes)...\n", request_len);
    total_written = 0;
    
    result = fn_write(handle, 0, (const uint8_t *)request, 
                      request_len, &bytes_written);
    if (result != FN_OK) {
        printf("Write failed: %s\n", fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    
    total_written += bytes_written;
    printf("Sent %u bytes: \"%.*s\"\n", 
           bytes_written, 
           bytes_written > 50 ? 50 : bytes_written, 
           request);
    
    /* Half-close: signal we're done writing.
     * This sends FIN to the peer, which is important for echo servers
     * to know when to stop reading and send their response.
     * 
     * To half-close, we write 0 bytes at the current write offset.
     * The device interprets this as a shutdown(SHUT_WR).
     */
    printf("Half-closing write side...\n");
    result = fn_write(handle, total_written, NULL, 0, &bytes_written);
    if (result != FN_OK && result != FN_ERR_UNSUPPORTED) {
        /* Some protocols may not support half-close, that's OK */
        printf("Half-close not supported or failed: %s (continuing)\n", 
               fn_error_string(result));
    }
    
    /* Read response with idle timeout.
     * 
     * This follows the same pattern as the Python client:
     * - Track idle time since last data received
     * - Stop when idle timeout expires
     * - Sleep briefly between reads to avoid busy looping
     */
    printf("\nReading response...\n");
    total_read = 0;
    
#ifndef __ATARI__
    double idle_timeout = (double)FN_IDLE_TIMEOUT_SECS;
    double idle_deadline = get_time_secs() + idle_timeout;
#else
    /* On Atari, use a simple counter instead of time */
    uint16_t idle_count = 0;
    const uint16_t max_idle_count = 100;  /* ~1 second at 10ms per iteration */
#endif
    
    while (1) {
        result = fn_read(handle, total_read, buffer, BUFFER_SIZE - 1, 
                         &bytes_read, &flags);
        
        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            /* Data not ready yet - check idle timeout */
#ifndef __ATARI__
            double now = get_time_secs();
            if (total_read > 0 && now >= idle_deadline) {
                printf("\n[Read complete - idle timeout]\n");
                break;
            }
            /* Sleep briefly to avoid busy looping (20ms like Python client) */
            usleep(20000);
#else
            /* On Atari, use counter-based idle detection */
            if (total_read > 0) {
                idle_count++;
                if (idle_count >= max_idle_count) {
                    printf("\n[Read complete - idle timeout]\n");
                    break;
                }
            }
#endif
            continue;
        }
        
        if (result == FN_ERR_TIMEOUT) {
            /* Timeout - if we've received data, consider it complete */
            if (total_read > 0) {
                printf("\n[Read complete - timeout]\n");
                break;
            }
            printf("\nRead timeout (no data received)\n");
            break;
        }
        
        if (result == FN_ERR_IO) {
            /* I/O error - often means peer closed connection.
             * If we've received data, treat as EOF.
             */
            if (total_read > 0) {
                printf("\n[Read complete - peer closed]\n");
                break;
            }
            printf("\nRead error: %s\n", fn_error_string(result));
            break;
        }
        
        if (result != FN_OK) {
            printf("\nRead error: %s\n", fn_error_string(result));
            break;
        }
        
        if (bytes_read == 0) {
            /* No more data */
            printf("\n[Read complete - no more data]\n");
            break;
        }
        
        /* Got data - reset idle timeout */
#ifndef __ATARI__
        idle_deadline = get_time_secs() + idle_timeout;
#else
        idle_count = 0;
#endif
        
        /* Null-terminate and print */
        buffer[bytes_read < BUFFER_SIZE ? bytes_read : BUFFER_SIZE - 1] = '\0';
        printf("%s", (char *)buffer);
        
        total_read += bytes_read;
        
        /* Check for EOF flag */
        if (flags & FN_READ_EOF) {
            printf("\n[EOF reached]\n");
            break;
        }
    }
    
    printf("\n\nTotal bytes read: %lu\n", (unsigned long)total_read);
    
    /* Close the connection */
    printf("Closing connection...\n");
    result = fn_close(handle);
    if (result != FN_OK) {
        printf("Close result: %s\n", fn_error_string(result));
    } else {
        printf("Connection closed.\n");
    }
    
    printf("\nDone.\n");
    return 0;
}
