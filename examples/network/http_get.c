/**
 * @file http_get.c
 * @brief Simple HTTP GET Example
 * 
 * Demonstrates how to use the fujinet-nio-lib to perform an HTTP GET request.
 * Sequential response bodies require the caller to keep a matching read cursor.
 * 
 * Configuration via environment variables (all platforms):
 *   FN_TEST_URL - URL to fetch (default: http://localhost:8080/get)
 *   FN_PORT     - Serial port device (default: /dev/ttyUSB0)
 * 
 * For cc65 targets (Atari, Apple, etc.), environment variables are populated
 * from compile-time defines since there's no shell environment:
 *   FN_TEST_URL - Compile with -DFN_DEFAULT_TEST_URL=\"http://your-server/path\"
 * 
 * Build for Linux:
 *   make TARGET=linux
 * 
 * Build for Atari:
 *   make TARGET=atari
 *   make TARGET=atari FN_DEFAULT_TEST_URL=\"http://your-server/path\"
 */

#include <stdio.h>
#ifndef __BBC__
#include <stdlib.h>
#endif
#include <string.h>
#ifdef __BBC__
#include <conio.h>
#endif
#include "fujinet-nio.h"

/* ============================================================================
 * Compile-time Configuration (can be overridden via CFLAGS)
 * ============================================================================ */

#ifndef FN_DEFAULT_TEST_URL
#define FN_DEFAULT_TEST_URL "http://localhost:8080/get"
#endif

/* Buffer for reading data */
#ifdef __BBC__
#define BUFFER_SIZE 128
#else
#define BUFFER_SIZE 512
#endif
static uint8_t buffer[BUFFER_SIZE];

#ifdef __BBC__
static void app_puts(const char *s)
{
    cputs(s);
}

static void app_nl(void)
{
    cputs("\r\n");
}
#endif

/* ============================================================================
 * URL Selection
 * ============================================================================ */

static const char *get_config_url(void)
{
#ifdef __BBC__
    return FN_DEFAULT_TEST_URL;
#else
    const char *url;

    url = getenv("FN_TEST_URL");
    if (url == NULL || url[0] == '\0') {
        return FN_DEFAULT_TEST_URL;
    }

    return url;
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
    uint8_t flags;
    uint32_t total_read;
    const char *url;
    
#ifdef __BBC__
    app_puts("HTTP GET");
    app_nl();
#else
    printf("FujiNet-NIO HTTP GET Example\n");
    printf("============================\n\n");
#endif
    
    url = get_config_url();
    
    /* Initialize the library */
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
    
    /* Check if device is ready */
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
    
    /* Open HTTP connection */
#ifndef __BBC__
    printf("Opening HTTP connection to: %s\n", url);
#endif
    result = fn_open(&handle, FN_METHOD_GET, url, 0);
    if (result != FN_OK) {
#ifdef __BBC__
        app_puts("Open fail");
        app_nl();
#else
        printf("Open failed: %s\n", fn_error_string(result));
#endif
        return 1;
    }

#ifndef __BBC__
    printf("Handle: %u\n", handle);

    {
        uint16_t http_status;
        uint32_t content_length;
        uint8_t info_flags;

        result = fn_info(handle, &http_status, &content_length, &info_flags);
        if (result == FN_OK) {
            if (info_flags & FN_INFO_HAS_STATUS) {
                printf("HTTP Status: %u\n", http_status);
            }
            if (info_flags & FN_INFO_HAS_LENGTH) {
                printf("Content-Length: %lu\n", (unsigned long)content_length);
            }
        }
    }

    printf("\nReading data...\n");
#endif
    total_read = 0;
    
    /* Read data in chunks */
    while (1) {
        result = fn_read(handle, total_read, buffer, BUFFER_SIZE - 1, 
                         &bytes_read, &flags);
        
        if (result == FN_ERR_NOT_READY || result == FN_ERR_BUSY) {
            /* Data not ready yet, wait and retry */
#ifndef __BBC__
            printf("Waiting for data...\n");
#endif
            continue;
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
            /* No more data */
            break;
        }
        
        /* Null-terminate and print */
        buffer[bytes_read < BUFFER_SIZE ? bytes_read : BUFFER_SIZE - 1] = '\0';
#ifdef __BBC__
        app_puts((char *)buffer);
#else
        printf("%s", (char *)buffer);
#endif
        
        total_read += bytes_read;
        
        /* Check for EOF */
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
    
    /* Close the connection */
#ifndef __BBC__
    printf("Closing connection...\n");
#endif
    result = fn_close(handle);
    if (result != FN_OK) {
#ifdef __BBC__
        app_puts("Close fail");
        app_nl();
#else
        printf("Close failed: %s\n", fn_error_string(result));
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
