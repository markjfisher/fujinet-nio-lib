/*
 * fn_transport.c - Linux/POSIX Transport Implementation
 *
 * Uses termios serial I/O to communicate with fujinet-nio.
 * Can connect to:
 *   - Real serial ports (e.g., /dev/ttyUSB0 for ESP32)
 *   - PTY devices (for POSIX fujinet-nio)
 *
 * Usage:
 *   Set FN_PORT environment variable to the device path, e.g.:
 *   FN_PORT=/dev/ttyUSB0 ./my_app
 *   FN_PORT=/dev/pts/2 ./my_app
 */

#define _POSIX_C_SOURCE 199309L  /* For nanosleep */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <sys/select.h>

#include "fujinet-nio.h"
#include "fn_platform.h"
#include "fn_protocol.h"
#include "fn_internal.h"

/* Default serial port */
#define DEFAULT_PORT    "/dev/ttyUSB0"
#define DEFAULT_BAUD    115200

/* Module state */
static int _fd = -1;
static struct termios _saved_termios;

/* Baud rate lookup */
static speed_t _get_baud(int baud) {
    switch (baud) {
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        default:     return B115200;
    }
}

/*
 * Initialize the transport.
 * Opens the serial port specified by FN_PORT env var, or /dev/ttyUSB0.
 */
uint8_t fn_transport_init(void) {
    const char *port;
    const char *baud_str;
    int baud;
    struct termios tio;
    
    if (_fd >= 0) {
        return FN_OK;  /* Already initialized */
    }
    
    port = getenv("FN_PORT");
    if (port == NULL || port[0] == '\0') {
        port = DEFAULT_PORT;
    }
    
    baud_str = getenv("FN_BAUD");
    if (baud_str != NULL && baud_str[0] != '\0') {
        baud = atoi(baud_str);
        if (baud <= 0) baud = DEFAULT_BAUD;
    } else {
        baud = DEFAULT_BAUD;
    }
    
    /* Open serial port */
    _fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (_fd < 0) {
        fprintf(stderr, "fn_transport: cannot open %s: %s\n", port, strerror(errno));
        return FN_ERR_NOT_FOUND;
    }
    
    /* Save current settings */
    if (tcgetattr(_fd, &_saved_termios) < 0) {
        fprintf(stderr, "fn_transport: tcgetattr failed: %s\n", strerror(errno));
        close(_fd);
        _fd = -1;
        return FN_ERR_IO;
    }
    
    /* Configure for raw binary I/O */
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = 0;  /* No input processing */
    tio.c_oflag = 0;  /* No output processing */
    tio.c_lflag = 0;  /* No local modes (no echo, no signals) */
    
    /* Set baud rate */
    cfsetispeed(&tio, _get_baud(baud));
    cfsetospeed(&tio, _get_baud(baud));
    
    /* Timeout: 0.1 seconds (VTIME in deciseconds) */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 1;
    
    if (tcsetattr(_fd, TCSANOW, &tio) < 0) {
        fprintf(stderr, "fn_transport: tcsetattr failed: %s\n", strerror(errno));
        close(_fd);
        _fd = -1;
        return FN_ERR_IO;
    }
    
    /* Flush any pending data */
    tcflush(_fd, TCIOFLUSH);
    
    return FN_OK;
}

/*
 * Check if transport is ready for communication.
 */
uint8_t fn_transport_ready(void) {
    if (_fd < 0) {
        return 0;
    }
    return 1;
}

/*
 * Exchange a FujiBus packet with the device.
 * Sends the request packet and receives the response.
 * 
 * request: FujiBus request packet (not SLIP-encoded)
 * req_len: length of request packet
 * response: buffer for response packet (SLIP-decoded)
 * resp_max: maximum response buffer size
 * resp_len: pointer to receive actual response length
 *
 * Returns: FN_OK on success, error code on failure
 */
uint8_t fn_transport_exchange(const uint8_t *request,
                               uint16_t req_len,
                               uint8_t *response,
                               uint16_t resp_max,
                               uint16_t *resp_len) {
    ssize_t n;
    uint16_t total;
    uint16_t timeout_ms;
    fd_set read_fds;
    fd_set write_fds;
    struct timeval tv;
    int ret;
    uint8_t slip_buf[1024];
    uint16_t slip_len;
    uint8_t raw_buf[1024];
    uint16_t raw_len;
    uint16_t i;
    
    (void)resp_max;  /* Suppress unused parameter warning */
    
    if (_fd < 0) {
        return FN_ERR_NOT_FOUND;
    }
    
    if (request == NULL || req_len == 0 || response == NULL || resp_len == NULL) {
        return FN_ERR_INVALID;
    }
    
    /* SLIP-encode the request */
    slip_len = fn_slip_encode(request, req_len, slip_buf);
    if (slip_len == 0) {
        return FN_ERR_IO;
    }
    
    /* Debug: print request packet */
    fprintf(stderr, "DEBUG: Request packet (%d bytes): ", req_len);
    for (i = 0; i < req_len && i < 32; i++) {
        fprintf(stderr, "%02X ", request[i]);
    }
    fprintf(stderr, "\n");
    
    /* Send the SLIP-encoded request */
    total = 0;
    while (total < slip_len) {
        n = write(_fd, slip_buf + total, slip_len - total);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Wait for write ready */
                FD_ZERO(&write_fds);
                FD_SET(_fd, &write_fds);
                tv.tv_sec = 1;
                tv.tv_usec = 0;
                if (select(_fd + 1, NULL, &write_fds, NULL, &tv) <= 0) {
                    fprintf(stderr, "fn_transport: write timeout\n");
                    return FN_ERR_IO;
                }
                continue;
            }
            fprintf(stderr, "fn_transport: write error: %s\n", strerror(errno));
            return FN_ERR_IO;
        }
        total += (uint16_t)n;
    }
    
    /* Make sure data is sent */
    tcdrain(_fd);
    
    /* Flush any pending input (e.g., echoes) */
    tcflush(_fd, TCIFLUSH);
    
    /* Small delay to allow device to process request */
    {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 10000000;  /* 10ms */
        nanosleep(&ts, NULL);
    }
    
    /* Receive the SLIP-encoded response with timeout */
    raw_len = 0;
    timeout_ms = 5000;  /* 5 second overall timeout */
    
    while (raw_len < sizeof(raw_buf)) {
        /* Wait for data */
        FD_ZERO(&read_fds);
        FD_SET(_fd, &read_fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  /* 100ms */
        ret = select(_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (ret < 0) {
            fprintf(stderr, "fn_transport: select error\n");
            return FN_ERR_IO;
        }
        if (ret == 0) {
            /* Timeout - check if we have a complete SLIP frame */
            /* A valid frame needs at least 2 END markers: C0 ... C0 */
            if (raw_len >= 2 && raw_buf[0] == SLIP_END && raw_buf[raw_len - 1] == SLIP_END) {
                /* We have a complete frame */
                break;
            }
            /* If we have some data but not a complete frame, keep waiting */
            if (raw_len > 0) {
                fprintf(stderr, "DEBUG: Partial frame (%d bytes), waiting...\n", raw_len);
            }
            timeout_ms -= 100;
            if (timeout_ms == 0) {
                fprintf(stderr, "fn_transport: receive timeout\n");
                return FN_ERR_TIMEOUT;
            }
            continue;
        }
        
        /* Read available data */
        n = read(_fd, raw_buf + raw_len, sizeof(raw_buf) - raw_len);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            fprintf(stderr, "fn_transport: read error: %s\n", strerror(errno));
            return FN_ERR_IO;
        }
        if (n == 0) {
            /* EOF (PTY closed) */
            fprintf(stderr, "fn_transport: EOF\n");
            return FN_ERR_IO;
        }
        
        raw_len += (uint16_t)n;
        
        /* Check for end of SLIP frame - need at least 2 bytes (C0 ... C0) */
        if (raw_len >= 2 && raw_buf[raw_len - 1] == SLIP_END && raw_buf[0] == SLIP_END) {
            break;
        }
    }
    
    /* Debug: print raw response */
    fprintf(stderr, "DEBUG: Raw response (%d bytes): ", raw_len);
    for (i = 0; i < raw_len && i < 64; i++) {
        fprintf(stderr, "%02X ", raw_buf[i]);
    }
    fprintf(stderr, "\n");
    
    /* SLIP-decode the response */
    *resp_len = fn_slip_decode(raw_buf, raw_len, response);
    if (*resp_len == 0) {
        fprintf(stderr, "fn_transport: SLIP decode failed\n");
        return FN_ERR_IO;
    }
    
    /* Debug: print decoded response */
    fprintf(stderr, "DEBUG: Decoded response (%d bytes): ", *resp_len);
    for (i = 0; i < *resp_len && i < 64; i++) {
        fprintf(stderr, "%02X ", response[i]);
    }
    fprintf(stderr, "\n");
    
    return FN_OK;
}

/*
 * Close the transport.
 */
void fn_transport_close(void) {
    if (_fd >= 0) {
        /* Restore original termios settings */
        tcsetattr(_fd, TCSANOW, &_saved_termios);
        close(_fd);
        _fd = -1;
    }
}
