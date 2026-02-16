/**
 * @file http_get.c
 * @brief Simple HTTP GET Example
 * 
 * Demonstrates how to use the fujinet-nio-lib to perform an HTTP GET request.
 * 
 * Build for Atari:
 *   cl65 -t atari -I../include http_get.c ../build/fujinet-nio.atari.lib -o http_get.xex
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fujinet-nio.h"

/* Buffer for reading data */
#define BUFFER_SIZE 512
static uint8_t buffer[BUFFER_SIZE];

int main(void)
{
    uint8_t result;
    fn_handle_t handle;
    uint16_t bytes_read;
    uint8_t flags;
    uint16_t http_status;
    uint32_t content_length;
    uint8_t info_flags;
    uint32_t total_read;
    
    printf("FujiNet-NIO HTTP GET Example\n");
    printf("============================\n\n");
    
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
    
    /* Open HTTP connection */
    printf("Opening HTTP connection...\n");
    result = fn_open(&handle, FN_METHOD_GET, 
                     "https://fujinet.online/", 
                     FN_OPEN_TLS | FN_OPEN_FOLLOW_REDIR);
    if (result != FN_OK) {
        printf("Open failed: %s\n", fn_error_string(result));
        return 1;
    }
    
    printf("Handle: %u\n", handle);
    
    /* Get info */
    result = fn_info(handle, &http_status, &content_length, &info_flags);
    if (result == FN_OK) {
        if (info_flags & FN_INFO_HAS_STATUS) {
            printf("HTTP Status: %u\n", http_status);
        }
        if (info_flags & FN_INFO_HAS_LENGTH) {
            printf("Content-Length: %lu\n", (unsigned long)content_length);
        }
    }
    
    printf("\nReading data...\n");
    total_read = 0;
    
    /* Read data in chunks */
    while (1) {
        result = fn_read(handle, total_read, buffer, BUFFER_SIZE - 1, 
                         &bytes_read, &flags);
        
        if (result == FN_ERR_NOT_READY) {
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
        
        /* Check for EOF */
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
        printf("Close failed: %s\n", fn_error_string(result));
    }
    
    printf("\nDone.\n");
    return 0;
}
