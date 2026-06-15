/**
 * @file tcp_get.c
 * @brief TCP/TLS Client Example
 * 
 * Demonstrates TCP and TLS connections using fujinet-nio-lib.
 * Works on Linux and cc65 targets including Atari and BBC.
 * 
 * Configuration via environment variables (all platforms):
 *   FN_TEST_URL    - Full URL (e.g., tcp://host:port or tls://host:port)
 *   FN_TCP_HOST    - Host to connect to (default: "localhost")
 *   FN_TCP_PORT    - Port to connect to (default: "7777")
 *   FN_TCP_TLS     - Set to "1" to enable TLS (default: "0")
 *   FN_TCP_REQUEST - Request string to send (default: "Hello from FujiNet-NIO!\r\n")
 *   FN_PORT        - Serial port device (default: /dev/ttyUSB0)
 * 
 * For cc65 targets (Atari, Apple, etc.), environment variables are populated
 * from compile-time defines since there's no shell environment:
 *   FN_TCP_HOST    - Compile with -DFN_TCP_HOST=\"host\"
 *   FN_TCP_PORT    - Compile with -DFN_TCP_PORT=\"port\"
 *   FN_TCP_TLS     - Compile with -DFN_TCP_TLS=1
 *   FN_TCP_REQUEST - Compile with -DFN_TCP_REQUEST=\"request\"
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
 *   FN_TEST_URL="tls://echo.fujinet.online:6001" ./bin/linux/tcp_get
 */

/* Feature test macros MUST come before any includes */
#ifndef __CC65__
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

/* Standard includes (all platforms) */
#include <stdio.h>
#ifndef __BBC__
#include <stdlib.h>
#endif
#include <string.h>
#ifdef __BBC__
#include <conio.h>
#endif

/* Platform-specific includes */
#ifndef __CC65__
/* POSIX includes for Linux */
#include <time.h>
#include <unistd.h>
#endif

/* FujiNet library */
#include "fujinet-nio.h"

/* ============================================================================
 * Compile-time Configuration (can be overridden via CFLAGS)
 * 
 * These defaults are used on cc65 targets where there's no shell environment.
 * On Linux, these serve as fallbacks if environment variables aren't set.
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

#ifndef FN_TCP_REQUEST
#define FN_TCP_REQUEST "Hello from FujiNet-NIO!\r\n"
#endif

#ifndef FN_IDLE_TIMEOUT_SECS
#define FN_IDLE_TIMEOUT_SECS 1
#endif

/* ============================================================================
 * Static Buffers (for cc65 compatibility - no large stack allocations)
 * ============================================================================ */

#ifdef __BBC__
#define BUFFER_SIZE 128
#define URL_MAX_LEN 96
#else
#define BUFFER_SIZE 512
#define URL_MAX_LEN FN_MAX_URL_LEN
#endif

static uint8_t g_buffer[BUFFER_SIZE];
static char g_url[URL_MAX_LEN];

/* ============================================================================
 * cc65 Environment Setup
 * 
 * cc65 has getenv()/putenv() support, but no shell environment. We use
 * putenv() to populate environment variables from compile-time defines,
 * allowing the rest of the code to use getenv() uniformly.
 * 
 * Note: cc65's putenv() stores the string pointer directly (no copy),
 * so we use static storage for the environment strings.
 * ============================================================================ */

#ifdef __CC65__

#define FN_STRINGIFY_INNER(x) #x
#define FN_STRINGIFY(x) FN_STRINGIFY_INNER(x)

#ifndef __BBC__
/* Static storage for environment strings (putenv doesn't copy!) */
static char env_fn_tcp_host[] = "FN_TCP_HOST=" FN_TCP_HOST;
static char env_fn_tcp_port[] = "FN_TCP_PORT=" FN_TCP_PORT;
static char env_fn_tcp_tls[]  = "FN_TCP_TLS=" FN_STRINGIFY(FN_TCP_TLS);
static char env_fn_tcp_request[] = "FN_TCP_REQUEST=" FN_TCP_REQUEST;

/**
 * @brief Set up environment variables from compile-time defines for cc65.
 * 
 * This must be called at program start before any getenv() calls.
 */
static void setup_env(void)
{
    putenv(env_fn_tcp_host);
    putenv(env_fn_tcp_port);
    putenv(env_fn_tcp_tls);
    putenv(env_fn_tcp_request);
}
#endif

#endif /* __CC65__ */

#ifdef __BBC__
static void app_puts(const char *s)
{
    while (*s != '\0') {
        if (*s == '\n') {
            cputs("\r\n");
        } else if (*s != '\r') {
            cputc(*s);
        }
        ++s;
    }
}

static void app_nl(void)
{
    cputs("\r\n");
}
#endif

/* ============================================================================
 * Platform Abstraction: Time and Delay
 * ============================================================================ */

/**
 * @brief Idle timer state
 */
typedef struct {
    int count;          /* Iteration count (all platforms) */
#ifndef __CC65__
    long deadline_ms;   /* Deadline in milliseconds (POSIX only) */
#endif
} idle_timer_t;

/**
 * @brief Initialize idle timer
 */
static void idle_init(idle_timer_t *t)
{
    t->count = 0;
#ifndef __CC65__
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
    
#ifndef __CC65__
    {
        struct timespec ts;
        long now_ms;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        now_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        return (now_ms >= t->deadline_ms) ? 1 : 0;
    }
#else
    /* cc65 targets: count-based timeout (~100 iterations = a short grace period) */
    return (t->count >= 100) ? 1 : 0;
#endif
}

/**
 * @brief Reset idle timer after receiving data
 */
static void idle_reset(idle_timer_t *t)
{
    t->count = 0;
#ifndef __CC65__
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
#ifndef __CC65__
    usleep(20000);  /* 20ms */
#endif
    /* cc65 targets: no sleep, just retry */
}

/* ============================================================================
 * Configuration Functions
 * 
 * All platforms use getenv() to retrieve configuration. On cc65 targets,
 * setup_env() must be called first to populate the environment from
 * compile-time defines.
 * ============================================================================ */

/**
 * @brief Get configuration URL
 * 
 * Priority:
 *   1. FN_TEST_URL environment variable (full URL)
 *   2. FN_TCP_HOST/PORT/TLS environment variables
 *   3. Compile-time defines (via setup_env() on cc65)
 * 
 * @return URL string (points to g_url or environment variable)
 */
static const char *get_config_url(void)
{
#ifdef __BBC__
    if (FN_TCP_TLS) {
        strcpy(g_url, "tls://");
    } else {
        strcpy(g_url, "tcp://");
    }
    strcat(g_url, FN_TCP_HOST);
    strcat(g_url, ":");
    strcat(g_url, FN_TCP_PORT);
    return g_url;
#else
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
        snprintf(g_url, URL_MAX_LEN, "tls://%s:%s", host, port_str);
    } else {
        snprintf(g_url, URL_MAX_LEN, "tcp://%s:%s", host, port_str);
    }
    
    return g_url;
#endif
}

/**
 * @brief Get request data to send
 * @return Request string
 */
static const char *get_config_request(void)
{
#ifdef __BBC__
    return FN_TCP_REQUEST;
#else
    const char *req = getenv("FN_TCP_REQUEST");
    if (req != NULL && req[0] != '\0') {
        return req;
    }
    return FN_TCP_REQUEST;
#endif
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
    
#if defined(__CC65__) && !defined(__BBC__)
    /* Set up environment from compile-time defines for cc65 */
    setup_env();
#endif
    
    /* Print header */
#ifdef __BBC__
    app_puts("TCP GET");
    app_nl();
#else
    printf("FujiNet-NIO TCP/TLS Client Example\n");
    printf("==================================\n\n");
#endif
    
    /* Get configuration */
    url = get_config_url();
    request = get_config_request();
#ifndef __BBC__
    printf("URL: %s\n\n", url);
#endif
    
    /* Initialize library */
#ifndef __BBC__
    printf("Initializing...\n");
#endif
    result = fn_init();
    if (result != FN_OK) {
#ifdef __BBC__
        app_puts("Init fail");
        app_nl();
#else
        printf("Init failed: %s\n", fn_error_string(result));
#endif
        return 1;
    }
    
    if (!fn_is_ready()) {
#ifdef __BBC__
        app_puts("Not ready");
        app_nl();
#else
        printf("FujiNet device not ready!\n");
#endif
        return 1;
    }
#ifndef __BBC__
    printf("Device ready.\n\n");
#endif
    
    /* Open connection */
#ifndef __BBC__
    printf("Opening connection...\n");
#endif
    result = fn_open(&handle, 0, url, 0);
    if (result != FN_OK) {
#ifdef __BBC__
        app_puts("Open fail");
        app_nl();
#else
        printf("Connection failed: %s\n", fn_error_string(result));
#endif
        return 1;
    }
#ifndef __BBC__
    printf("Handle: %u\nConnection established.\n", handle);
#endif
    
    /* Send data */
    request_len = (uint16_t)strlen(request);
#ifndef __BBC__
    printf("\nSending data (%u bytes)...\n", request_len);
#endif
    
    result = fn_write(handle, 0, (const uint8_t *)request, request_len, &bytes_written);
    if (result != FN_OK) {
#ifdef __BBC__
        app_puts("Write fail");
        app_nl();
#else
        printf("Write failed: %s\n", fn_error_string(result));
#endif
        fn_close(handle);
        return 1;
    }
    total_written = bytes_written;
#ifndef __BBC__
    printf("Sent %u bytes: \"%.*s\"\n", bytes_written, 
           bytes_written > 50 ? 50 : bytes_written, request);
#endif
    
    /* Half-close write side (signals FIN to peer) */
#ifndef __BBC__
    printf("Half-closing write side...\n");
#endif
    result = fn_write(handle, total_written, NULL, 0, &bytes_written);
    if (result != FN_OK && result != FN_ERR_UNSUPPORTED) {
#ifndef __BBC__
        printf("Half-close: %s (continuing)\n", fn_error_string(result));
#endif
    }
    
    /* Read response with idle timeout */
#ifndef __BBC__
    printf("\nReading response...\n");
#endif
    total_read = 0;
    idle_init(&idle);
    
    for (;;) {
        result = fn_read(handle, total_read, g_buffer, BUFFER_SIZE - 1,
                         &bytes_read, &flags);
        
        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            /* Data not ready - check idle timeout if we've received data */
            if (total_read > 0 && idle_expired(&idle)) {
#ifdef __BBC__
                app_nl();
#else
                printf("\n[Read complete - idle timeout]\n");
#endif
                break;
            }
            sleep_brief();
            continue;
        }
        
        if (result == FN_ERR_TIMEOUT) {
            if (total_read > 0) {
#ifdef __BBC__
                app_nl();
#else
                printf("\n[Read complete - timeout]\n");
#endif
            } else {
#ifdef __BBC__
                app_puts("Read timeout");
                app_nl();
#else
                printf("\nRead timeout (no data received)\n");
#endif
            }
            break;
        }
        
        if (result == FN_ERR_IO) {
            if (total_read > 0) {
#ifdef __BBC__
                app_nl();
#else
                printf("\n[Read complete - peer closed]\n");
#endif
            } else {
#ifdef __BBC__
                app_puts("Read fail");
                app_nl();
#else
                printf("\nRead error: %s\n", fn_error_string(result));
#endif
            }
            break;
        }
        
        if (result != FN_OK) {
#ifdef __BBC__
            app_puts("Read fail");
            app_nl();
#else
            printf("\nRead error: %s\n", fn_error_string(result));
#endif
            break;
        }
        
        if (bytes_read == 0) {
#ifdef __BBC__
            app_nl();
#else
            printf("\n[Read complete - no more data]\n");
#endif
            break;
        }
        
        /* Got data - reset idle timer and print */
        idle_reset(&idle);
        g_buffer[bytes_read < BUFFER_SIZE ? bytes_read : BUFFER_SIZE - 1] = '\0';
#ifdef __BBC__
        app_puts((char *)g_buffer);
#else
        printf("%s", (char *)g_buffer);
#endif
        total_read += (uint32_t)bytes_read;
        
        if (flags & FN_READ_EOF) {
#ifdef __BBC__
            app_nl();
#else
            printf("\n[EOF reached]\n");
#endif
            break;
        }
    }
    
#ifndef __BBC__
    printf("\n\nTotal bytes read: %lu\n", (unsigned long)total_read);
#endif
    
    /* Close connection */
#ifndef __BBC__
    printf("Closing connection...\n");
#endif
    result = fn_close(handle);
    if (result != FN_OK) {
#ifdef __BBC__
        app_puts("Close fail");
        app_nl();
#else
        printf("Close result: %s\n", fn_error_string(result));
    } else {
        printf("Connection closed.\n");
#endif
    }
    
#ifdef __BBC__
    app_nl();
    app_puts("Done");
    app_nl();
#else
    printf("\nDone.\n");
#endif
    return 0;
}
