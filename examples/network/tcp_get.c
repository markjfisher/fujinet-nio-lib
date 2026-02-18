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
 *     FN_TCP_PORT - Port to connect to (default: 8080)
 *     FN_TCP_TLS  - Set to "1" to enable TLS (default: disabled)
 *     FN_PORT     - Serial port device (default: /dev/ttyUSB0)
 * 
 *   For Atari, modify defaults at compile time:
 *     make TARGET=atari CFLAGS="-DFN_DEFAULT_TCP_HOST=\\\"your-server\\\" -DFN_DEFAULT_TCP_PORT=443"
 * 
 * Build for Linux:
 *   make TARGET=linux tcp_get
 * 
 * Build for Atari:
 *   make TARGET=atari tcp_get
 * 
 * Example:
 *   # Connect to local echo server
 *   ./bin/linux/tcp_get
 * 
 *   # Connect to remote server with TLS
 *   FN_TCP_HOST=example.com FN_TCP_PORT=443 FN_TCP_TLS=1 ./bin/linux/tcp_get
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fujinet-nio.h"

/* Default configuration - can be overridden via build defines */
#ifndef FN_DEFAULT_TCP_HOST
#define FN_DEFAULT_TCP_HOST "localhost"
#endif

#ifndef FN_DEFAULT_TCP_PORT
#define FN_DEFAULT_TCP_PORT 8080
#endif

/* Buffer for reading data - static for cc65 compatibility */
#define BUFFER_SIZE 512
static uint8_t buffer[BUFFER_SIZE];

/* URL buffer for TLS connections - static for cc65 compatibility */
#define URL_MAX_LEN 256
static char url_buffer[URL_MAX_LEN];

/* Simple request to send (HTTP-like for testing) */
static const char *default_request = 
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Connection: close\r\n"
    "\r\n";

int main(void)
{
    uint8_t result;
    fn_handle_t handle;
    uint16_t bytes_read;
    uint16_t bytes_written;
    uint8_t flags;
    uint8_t info_flags;
    uint32_t total_read;
    uint16_t total_written;
    uint16_t http_status;
    uint32_t content_length;
    const char *host;
    uint16_t port;
    uint8_t use_tls;
    const char *request;
    uint16_t request_len;
    
    printf("FujiNet-NIO TCP/TLS Client Example\n");
    printf("==================================\n\n");
    
    /* Get configuration from environment or use defaults */
    /* Note: getenv() not available on Atari, use compile-time defaults */
#ifdef __ATARI__
    host = FN_DEFAULT_TCP_HOST;
    port = FN_DEFAULT_TCP_PORT;
    use_tls = 0;
    request = default_request;
#else
    const char *port_str;
    const char *tls_str;
    
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
    
    /* Get custom request from environment, or use default */
    request = getenv("FN_TCP_REQUEST");
    if (request == NULL || request[0] == '\0') {
        request = default_request;
    }
#endif
    
    printf("Configuration:\n");
    printf("  Host: %s\n", host);
    printf("  Port: %u\n", port);
    printf("  TLS:  %s\n", use_tls ? "enabled" : "disabled");
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
    
    /* Open TCP connection */
    printf("Opening %s connection to %s:%u...\n", 
           use_tls ? "TLS" : "TCP", host, port);
    
    if (use_tls) {
        /* For TLS, use fn_open with FN_OPEN_TLS flag */
        snprintf(url_buffer, URL_MAX_LEN, "tcp://%s:%u", host, port);
        result = fn_open(&handle, 0, url_buffer, FN_OPEN_TLS);
    } else {
        /* For plain TCP, use convenience function */
        result = fn_tcp_open(&handle, host, port);
    }
    
    if (result != FN_OK) {
        printf("Connection failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    printf("Handle: %u\n", handle);
    
    /* Wait for connection to be established */
    printf("Waiting for connection...\n");
    while (1) {
        result = fn_info(handle, &http_status, &content_length, &info_flags);
        if (result == FN_OK && (info_flags & FN_INFO_CONNECTED)) {
            printf("Connected!\n");
            break;
        }
        if (result != FN_OK && result != FN_ERR_NOT_READY) {
            printf("Connection error: %s\n", fn_error_string(result));
            fn_close(handle);
            return 1;
        }
        /* Small delay would go here in a real application */
    }
    
    /* Send request */
    request_len = (uint16_t)strlen(request);
    printf("\nSending request (%u bytes)...\n", request_len);
    total_written = 0;
    
    result = fn_write(handle, 0, (const uint8_t *)request, 
                      request_len, &bytes_written);
    if (result != FN_OK) {
        printf("Write failed: %s\n", fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    
    total_written += bytes_written;
    printf("Sent %u bytes\n", bytes_written);
    
    /* Read response */
    printf("\nReading response...\n");
    total_read = 0;
    
    while (1) {
        result = fn_read(handle, total_read, buffer, BUFFER_SIZE - 1, 
                         &bytes_read, &flags);
        
        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            /* Data not ready yet, wait and retry */
            continue;
        }
        
        if (result != FN_OK) {
            printf("\nRead error: %s\n", fn_error_string(result));
            break;
        }
        
        if (bytes_read == 0) {
            /* No more data */
            break;
        }
        
        /* Null-terminate and print */
        buffer[bytes_read < BUFFER_SIZE ? bytes_read : BUFFER_SIZE - 1] = '\0';
        printf("%s", (char *)buffer);
        
        total_read += bytes_read;
        
        /* Check for peer close or EOF */
        if (flags & FN_READ_EOF) {
            printf("\n[EOF reached]\n");
            break;
        }
        
        /* Check if peer closed connection */
        result = fn_info(handle, &http_status, &content_length, &info_flags);
        if (result == FN_OK && (info_flags & FN_INFO_PEER_CLOSED)) {
            printf("\n[Peer closed connection]\n");
            break;
        }
    }
    
    printf("\n\nTotal bytes read: %lu\n", (unsigned long)total_read);
    
    /* Close the connection */
    printf("Closing connection...\n");
    result = fn_close(handle);
    if (result != FN_OK) {
        printf("Close failed: %s\n", fn_error_string(result));
    }
    
    printf("\nDone.\n");
    return 0;
}
