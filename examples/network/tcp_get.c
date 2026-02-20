/**
 * @file tcp_get.c
 * @brief TCP/TLS Client Example
 * 
 * Demonstrates TCP and TLS connections using fujinet-nio-lib.
 * Works identically on Linux and Atari platforms.
 * 
 * Configuration:
 *   Compile-time defines (all platforms):
 *     FN_TCP_HOST - Host to connect to (default: "localhost")
 *     FN_TCP_PORT - Port to connect to (default: "7777")
 *     FN_TCP_TLS  - Set to 1 to enable TLS (default: 0)
 * 
 *   Runtime environment variables (Linux only):
 *     FN_TEST_URL    - Full URL (e.g., tcp://host:port or tls://host:port?testca=1)
 *     FN_TCP_HOST    - Overrides compile-time default
 *     FN_TCP_PORT    - Overrides compile-time default
 *     FN_TCP_TLS     - Overrides compile-time default ("1" to enable)
 *     FN_PORT        - Serial port device (default: /dev/ttyUSB0)
 * 
 * Build:
 *   make TARGET=linux tcp_get
 *   make TARGET=atari tcp_get
 *   make TARGET=atari FN_TCP_HOST=\"example.com\" FN_TCP_PORT=\"443\" FN_TCP_TLS=1 tcp_get
 * 
 * Examples:
 *   # TCP echo (using defaults)
 *   ./bin/linux/tcp_get
 * 
 *   # TLS with test CA (runtime override)
 *   FN_TCP_HOST=127.0.0.1 FN_TCP_PORT=7778 FN_TCP_TLS=1 ./bin/linux/tcp_get
 * 
 *   # Full URL (runtime override)
 *   FN_TEST_URL="tls://echo.fujinet.online:6001?testca=1" ./bin/linux/tcp_get
 */

/* Feature test macros MUST come before any includes */
#ifndef __ATARI__
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

/* Standard includes (all platforms) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Platform-specific includes */
#ifndef __ATARI__
/* POSIX includes for Linux */
#include <time.h>
#include <unistd.h>
#endif

/* FujiNet library */
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

#ifndef FN_TCP_TLS
#define FN_TCP_TLS 0
#endif

#ifndef FN_IDLE_TIMEOUT_SECS
#define FN_IDLE_TIMEOUT_SECS 1
#endif

/* ============================================================================
 * Static Buffers (for cc65 compatibility - no large stack allocations)
 * ============================================================================ */

#define BUFFER_SIZE 512
#define URL_MAX_LEN 256

static uint8_t g_buffer[BUFFER_SIZE];
static char g_url[URL_MAX_LEN];

/* ============================================================================
 * Platform Abstraction: Time and Delay
 * ============================================================================ */

/**
 * @brief Idle timer state
 */
typedef struct {
    int count;          /* Iteration count (all platforms) */
#ifndef __ATARI__
    long deadline_ms;   /* Deadline in milliseconds (POSIX only) */
#endif
} idle_timer_t;

/**
 * @brief Initialize idle timer
 */
static void idle_init(idle_timer_t *t)
{
    t->count = 0;
#ifndef __ATARI__
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        t->deadline_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000 
                       + (FN_IDLE_TIMEOUT_SECS * 1000);
    }
#endif
}

/**
 * @brief Check if idle timeout has expired
 * @return 1 if expired, 0 if not
 */
static int idle_expired(idle_timer_t *t)
{
    t->count++;
    
#ifndef __ATARI__
    {
        struct timespec ts;
        long now_ms;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        now_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        return (now_ms >= t->deadline_ms) ? 1 : 0;
    }
#else
    /* Atari: count-based timeout (~100 iterations = ~2 seconds) */
    return (t->count >= 100) ? 1 : 0;
#endif
}

/**
 * @brief Reset idle timer after receiving data
 */
static void idle_reset(idle_timer_t *t)
{
    t->count = 0;
#ifndef __ATARI__
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        t->deadline_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000 
                       + (FN_IDLE_TIMEOUT_SECS * 1000);
    }
#endif
}

/**
 * @brief Brief sleep to avoid busy looping
 */
static void sleep_brief(void)
{
#ifndef __ATARI__
    usleep(20000);  /* 20ms */
#endif
    /* Atari: no sleep, just retry */
}

/* ============================================================================
 * Platform Abstraction: Configuration
 * ============================================================================ */

/**
 * @brief Get configuration URL
 * 
 * Priority (POSIX only):
 *   1. FN_TEST_URL environment variable (full URL)
 *   2. FN_TCP_HOST/PORT/TLS environment variables
 *   3. Compile-time defines
 * 
 * For Atari, only compile-time defines are used.
 * 
 * @return URL string (points to g_url or environment variable)
 */
static const char *get_config_url(void)
{
#ifndef __ATARI__
    const char *url;
    const char *host;
    const char *port_str;
    const char *tls_str;
    int use_tls;
    
    /* Priority 1: Full URL from environment */
    url = getenv("FN_TEST_URL");
    if (url != NULL && url[0] != '\0') {
        return url;
    }
    
    /* Priority 2: Individual environment variables */
    host = getenv("FN_TCP_HOST");
    if (host == NULL || host[0] == '\0') {
        /* Priority 3: Compile-time default */
        host = FN_TCP_HOST;
    }
    
    port_str = getenv("FN_TCP_PORT");
    if (port_str == NULL || port_str[0] == '\0') {
        port_str = FN_TCP_PORT;
    }
    
    tls_str = getenv("FN_TCP_TLS");
    if (tls_str != NULL && tls_str[0] == '1') {
        use_tls = 1;
    } else {
        use_tls = FN_TCP_TLS;
    }
    
    /* Build URL */
    if (use_tls) {
        snprintf(g_url, URL_MAX_LEN, "tls://%s:%s?testca=1", host, port_str);
    } else {
        snprintf(g_url, URL_MAX_LEN, "tcp://%s:%s", host, port_str);
    }
    
    return g_url;
#else
    /* Atari: Use compile-time defines only */
    #if FN_TCP_TLS
    snprintf(g_url, URL_MAX_LEN, "tls://%s:%s?testca=1", FN_TCP_HOST, FN_TCP_PORT);
    #else
    snprintf(g_url, URL_MAX_LEN, "tcp://%s:%s", FN_TCP_HOST, FN_TCP_PORT);
    #endif
    return g_url;
#endif
}

/**
 * @brief Get request data to send
 * @return Request string
 */
static const char *get_config_request(void)
{
#ifndef __ATARI__
    const char *req = getenv("FN_TCP_REQUEST");
    if (req != NULL && req[0] != '\0') {
        return req;
    }
#endif
    return "Hello from FujiNet-NIO!\r\n";
}

/* ============================================================================
 * Main Application
 * ============================================================================ */

int main(void)
{
    uint8_t result;
    fn_handle_t handle;
    uint16_t bytes_read;
    uint16_t bytes_written;
    uint8_t flags;
    uint32_t total_read;
    uint16_t total_written;
    uint16_t request_len;
    const char *url;
    const char *request;
    idle_timer_t idle;
    
    /* Print header */
    printf("FujiNet-NIO TCP/TLS Client Example\n");
    printf("==================================\n\n");
    
    /* Get configuration */
    url = get_config_url();
    request = get_config_request();
    printf("URL: %s\n\n", url);
    
    /* Initialize library */
    printf("Initializing...\n");
    result = fn_init();
    if (result != FN_OK) {
        printf("Init failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    if (!fn_is_ready()) {
        printf("FujiNet device not ready!\n");
        return 1;
    }
    printf("Device ready.\n\n");
    
    /* Open connection */
    printf("Opening connection...\n");
    result = fn_open(&handle, 0, url, 0);
    if (result != FN_OK) {
        printf("Connection failed: %s\n", fn_error_string(result));
        return 1;
    }
    printf("Handle: %u\nConnection established.\n", handle);
    
    /* Send data */
    request_len = (uint16_t)strlen(request);
    printf("\nSending data (%u bytes)...\n", request_len);
    
    result = fn_write(handle, 0, (const uint8_t *)request, request_len, &bytes_written);
    if (result != FN_OK) {
        printf("Write failed: %s\n", fn_error_string(result));
        fn_close(handle);
        return 1;
    }
    total_written = bytes_written;
    printf("Sent %u bytes: \"%.*s\"\n", bytes_written, 
           bytes_written > 50 ? 50 : bytes_written, request);
    
    /* Half-close write side (signals FIN to peer) */
    printf("Half-closing write side...\n");
    result = fn_write(handle, total_written, NULL, 0, &bytes_written);
    if (result != FN_OK && result != FN_ERR_UNSUPPORTED) {
        printf("Half-close: %s (continuing)\n", fn_error_string(result));
    }
    
    /* Read response with idle timeout */
    printf("\nReading response...\n");
    total_read = 0;
    idle_init(&idle);
    
    for (;;) {
        result = fn_read(handle, total_read, g_buffer, BUFFER_SIZE - 1, 
                         &bytes_read, &flags);
        
        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            /* Data not ready - check idle timeout if we've received data */
            if (total_read > 0 && idle_expired(&idle)) {
                printf("\n[Read complete - idle timeout]\n");
                break;
            }
            sleep_brief();
            continue;
        }
        
        if (result == FN_ERR_TIMEOUT) {
            if (total_read > 0) {
                printf("\n[Read complete - timeout]\n");
            } else {
                printf("\nRead timeout (no data received)\n");
            }
            break;
        }
        
        if (result == FN_ERR_IO) {
            if (total_read > 0) {
                printf("\n[Read complete - peer closed]\n");
            } else {
                printf("\nRead error: %s\n", fn_error_string(result));
            }
            break;
        }
        
        if (result != FN_OK) {
            printf("\nRead error: %s\n", fn_error_string(result));
            break;
        }
        
        if (bytes_read == 0) {
            printf("\n[Read complete - no more data]\n");
            break;
        }
        
        /* Got data - reset idle timer and print */
        idle_reset(&idle);
        g_buffer[bytes_read < BUFFER_SIZE ? bytes_read : BUFFER_SIZE - 1] = '\0';
        printf("%s", (char *)g_buffer);
        total_read += bytes_read;
        
        if (flags & FN_READ_EOF) {
            printf("\n[EOF reached]\n");
            break;
        }
    }
    
    printf("\n\nTotal bytes read: %lu\n", (unsigned long)total_read);
    
    /* Close connection */
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
