/**
 * @file clock_test.c
 * @brief FujiNet Clock Device Example
 * 
 * Demonstrates how to use the fujinet-nio-lib clock device functions.
 * This example shows how to:
 *   - Get the current time from FujiNet
 *   - Set the time on FujiNet
 *   - Display time in human-readable format
 * 
 * Usage:
 *   Set environment variables to configure:
 *     FN_PORT     - Serial port device (default: /dev/ttyUSB0)
 * 
 * Build for Linux:
 *   make TARGET=linux
 * 
 * Build for Atari:
 *   make TARGET=atari
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fujinet-nio.h"

/* ============================================================================
 * Time Formatting Helpers (cc65 compatible - no floating point)
 * ============================================================================
 */

/* Days per month (non-leap year) */
static const uint8_t days_per_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* Month names */
static const char *month_names[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/**
 * Check if a year is a leap year.
 */
static int is_leap_year(uint16_t year)
{
    if ((year % 4) != 0) return 0;
    if ((year % 100) != 0) return 1;
    if ((year % 400) != 0) return 0;
    return 1;
}

/**
 * Convert 8-byte little-endian time to components.
 * 
 * @param time_bytes  8-byte array (little-endian)
 * @param year        Output: year (e.g., 2024)
 * @param month       Output: month (1-12)
 * @param day         Output: day (1-31)
 * @param hour        Output: hour (0-23)
 * @param minute      Output: minute (0-59)
 * @param second      Output: second (0-59)
 */
static void time_to_datetime(const uint8_t *time_bytes,
                             uint16_t *year,
                             uint8_t *month,
                             uint8_t *day,
                             uint8_t *hour,
                             uint8_t *minute,
                             uint8_t *second)
{
    uint32_t days;
    uint32_t secs;
    uint16_t y;
    uint8_t m;
    uint8_t d;
    uint16_t days_in_year;
    uint8_t days_in_month;
    uint8_t i;
    
    /* Extract seconds from little-endian 8-byte array */
    /* We only use the low 32 bits (good until 2106) */
    secs = 0;
    for (i = 0; i < 4; i++) {
        secs |= ((uint32_t)time_bytes[i]) << (8 * i);
    }
    
    /* Extract time components */
    *second = (uint8_t)(secs % 60);
    secs /= 60;
    *minute = (uint8_t)(secs % 60);
    secs /= 60;
    *hour = (uint8_t)(secs % 24);
    secs /= 24;
    
    /* Now secs is days since 1970-01-01 */
    days = secs;
    
    /* Calculate year */
    y = 1970;
    for (;;) {
        days_in_year = is_leap_year(y) ? 366 : 365;
        if (days < days_in_year) {
            break;
        }
        days -= days_in_year;
        y++;
    }
    *year = y;
    
    /* Calculate month and day */
    d = (uint8_t)(days + 1);  /* Days are 1-indexed */
    
    for (m = 0; m < 12; m++) {
        days_in_month = days_per_month[m];
        if (m == 1 && is_leap_year(y)) {
            days_in_month = 29;  /* February in leap year */
        }
        if (d <= days_in_month) {
            break;
        }
        d -= days_in_month;
    }
    
    *month = (uint8_t)(m + 1);  /* Months are 1-indexed */
    *day = d;
}

/**
 * Print a timestamp in human-readable format.
 */
static void print_time(FN_TIME_T *time)
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    const char *month_name;
    
#ifdef __CC65__
    time_to_datetime(time->b, &year, &month, &day, &hour, &minute, &second);
#else
    uint8_t bytes[8];
    uint8_t i;
    uint64_t t = *time;
    for (i = 0; i < 8; i++) {
        bytes[i] = (uint8_t)((t >> (8 * i)) & 0xFF);
    }
    time_to_datetime(bytes, &year, &month, &day, &hour, &minute, &second);
#endif
    
    month_name = (month >= 1 && month <= 12) ? month_names[month - 1] : "???";
    
    printf("%04u-%s-%02u %02u:%02u:%02u UTC\n",
           (unsigned)year, month_name, (unsigned)day,
           (unsigned)hour, (unsigned)minute, (unsigned)second);
}

/**
 * Create a time value from a Unix timestamp.
 */
static void make_time(FN_TIME_T *time, uint32_t timestamp)
{
#ifdef __CC65__
    uint8_t i;
    for (i = 0; i < 4; i++) {
        time->b[i] = (uint8_t)((timestamp >> (8 * i)) & 0xFF);
    }
    /* Zero the high bytes */
    for (i = 4; i < 8; i++) {
        time->b[i] = 0;
    }
#else
    *time = (uint64_t)timestamp;
#endif
}

/**
 * Print raw time bytes for debugging.
 */
static void print_time_raw(FN_TIME_T *time)
{
    uint8_t i;
    printf("Raw bytes: ");
#ifdef __CC65__
    for (i = 0; i < 8; i++) {
        printf("%02X ", time->b[i]);
    }
#else
    uint64_t t = *time;
    for (i = 0; i < 8; i++) {
        printf("%02X ", (unsigned)((t >> (8 * i)) & 0xFF));
    }
#endif
    printf("\n");
}

/* ============================================================================
 * Main Program
 * ============================================================================
 */

int main(void)
{
    uint8_t result;
    FN_TIME_T current_time;
    FN_TIME_T test_time;
    
    printf("FujiNet-NIO Clock Device Example\n");
    printf("================================\n\n");
    
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
    
    /* Get current time */
    printf("Getting current time...\n");
    result = fn_clock_get(&current_time);
    if (result != FN_OK) {
        printf("Failed to get time: %s\n", fn_error_string(result));
        if (result == FN_ERR_NOT_READY) {
            printf("(Time may not be synchronized - check WiFi/NTP)\n");
        }
    } else {
        printf("Current time:\n  ");
        print_time(&current_time);
        print_time_raw(&current_time);
    }
    
    printf("\n");
    
    /* Set time to a test value (2024-01-01 00:00:00 UTC = 1704067200) */
    /* Note: This is just for demonstration - setting the time may not */
    /* be allowed on all FujiNet configurations */
    printf("Testing time set...\n");
    make_time(&test_time, 1704067200UL);  /* 2024-01-01 00:00:00 UTC */
    printf("Setting time to: 2024-01-01 00:00:00 UTC\n");
    
    result = fn_clock_set(&test_time);
    if (result != FN_OK) {
        printf("Failed to set time: %s\n", fn_error_string(result));
        printf("(This may be expected if time setting is disabled)\n");
    } else {
        printf("Time set successfully.\n");
    }
    
    printf("\n");
    
    /* Get the time back to verify */
    printf("Getting time back...\n");
    result = fn_clock_get(&current_time);
    if (result != FN_OK) {
        printf("Failed to get time: %s\n", fn_error_string(result));
    } else {
        printf("Current time:\n  ");
        print_time(&current_time);
        print_time_raw(&current_time);
    }
    
    printf("\nDone.\n");
    return 0;
}
