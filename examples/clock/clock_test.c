/**
 * @file clock_test.c
 * @brief FujiNet Clock Device Example
 * 
 * Demonstrates how to use the fujinet-nio-lib clock device functions.
 * This example shows how to:
 *   - Get the current time from FujiNet in various formats
 *   - Set the time on FujiNet
 *   - Get/Set timezone
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
    char iso_time[FN_MAX_TIME_STRING];
    char tz_buf[FN_MAX_TIMEZONE_LEN];
    uint8_t binary_time[16];
    uint8_t i;
    
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
    
    /* ========================================================================
     * Test 1: Get current time (raw format)
     * ======================================================================== */
    printf("--- Test 1: Get Current Time (Raw) ---\n");
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
    
    /* ========================================================================
     * Test 2: Get time in ISO 8601 format (UTC)
     * ======================================================================== */
    printf("--- Test 2: Get Time (ISO 8601 UTC) ---\n");
    result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_UTC_ISO);
    if (result == FN_OK) {
        printf("UTC time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 3: Get time in ISO 8601 format (with timezone)
     * ======================================================================== */
    printf("--- Test 3: Get Time (ISO 8601 with TZ) ---\n");
    result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_TZ_ISO);
    if (result == FN_OK) {
        printf("Local time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 4: Get current timezone
     * ======================================================================== */
    printf("--- Test 4: Get Current Timezone ---\n");
    result = fn_clock_get_timezone(tz_buf);
    if (result == FN_OK) {
        printf("Current timezone: %s\n", tz_buf);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 5: Get time in binary formats
     * ======================================================================== */
    printf("--- Test 5: Binary Formats ---\n");
    
    /* Simple binary (7 bytes) */
    result = fn_clock_get_format(binary_time, FN_TIME_FORMAT_SIMPLE);
    if (result == FN_OK) {
        printf("Simple binary (7 bytes): ");
        for (i = 0; i < 7; i++) {
            printf("%02X ", binary_time[i]);
        }
        printf("\n");
        printf("  -> %04d-%02d-%02d %02d:%02d:%02d\n",
               (int)binary_time[0] * 100 + (int)binary_time[1],
               (int)binary_time[2], (int)binary_time[3],
               (int)binary_time[4], (int)binary_time[5], (int)binary_time[6]);
    }
    
    /* ProDOS binary (4 bytes) */
    result = fn_clock_get_format(binary_time, FN_TIME_FORMAT_PRODOS);
    if (result == FN_OK) {
        printf("ProDOS binary (4 bytes): ");
        for (i = 0; i < 4; i++) {
            printf("%02X ", binary_time[i]);
        }
        printf("\n");
    }
    
    /* ApeTime binary (6 bytes) */
    result = fn_clock_get_format(binary_time, FN_TIME_FORMAT_APETIME);
    if (result == FN_OK) {
        printf("ApeTime binary (6 bytes): ");
        for (i = 0; i < 6; i++) {
            printf("%02X ", binary_time[i]);
        }
        printf("\n");
    }
    printf("\n");
    
    /* ========================================================================
     * Test 6: Get time for a specific timezone
     * ======================================================================== */
    printf("--- Test 6: Time for Specific Timezone ---\n");
    
    /* Try Pacific Time */
    result = fn_clock_get_tz((uint8_t*)iso_time, "PST8PDT,M3.2.0,M11.1.0", FN_TIME_FORMAT_TZ_ISO);
    if (result == FN_OK) {
        printf("Pacific Time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    
    /* Try Central European Time */
    result = fn_clock_get_tz((uint8_t*)iso_time, "CET-1CEST,M3.5.0,M10.5.0/3", FN_TIME_FORMAT_TZ_ISO);
    if (result == FN_OK) {
        printf("Central European Time: %s\n", iso_time);
    } else {
        printf("Failed: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 7: Set timezone (non-persistent)
     * ======================================================================== */
    printf("--- Test 7: Set Timezone (non-persistent) ---\n");
    result = fn_clock_set_timezone("EST5EDT,M3.2.0,M11.1.0");
    if (result == FN_OK) {
        printf("Timezone set to EST5EDT\n");
        
        /* Get time in new timezone */
        result = fn_clock_get_format((uint8_t*)iso_time, FN_TIME_FORMAT_TZ_ISO);
        if (result == FN_OK) {
            printf("Eastern Time: %s\n", iso_time);
        }
        
        /* Get current timezone to verify */
        result = fn_clock_get_timezone(tz_buf);
        if (result == FN_OK) {
            printf("Current timezone is now: %s\n", tz_buf);
        }
    } else {
        printf("Failed to set timezone: %s\n", fn_error_string(result));
    }
    printf("\n");
    
    /* ========================================================================
     * Test 8: Set time (demonstration only)
     * ======================================================================== */
    printf("--- Test 8: Set Time (demonstration) ---\n");
    /* Set time to a test value (2024-01-01 00:00:00 UTC = 1704067200) */
    /* Note: This is just for demonstration - setting the time may not */
    /* be allowed on all FujiNet configurations */
    make_time(&test_time, 1704067200UL);  /* 2024-01-01 00:00:00 UTC */
    printf("Setting time to: 2024-01-01 00:00:00 UTC\n");
    
    result = fn_clock_set(&test_time);
    if (result != FN_OK) {
        printf("Failed to set time: %s\n", fn_error_string(result));
        printf("(This may be expected if time setting is disabled)\n");
    } else {
        printf("Time set successfully.\n");
    }
    
    /* Get the time back to verify */
    result = fn_clock_get(&current_time);
    if (result == FN_OK) {
        printf("Current time:\n  ");
        print_time(&current_time);
    }
    printf("\n");
    
    /* ========================================================================
     * Test 9: Sync network time (restore from NTP)
     * ======================================================================== */
    printf("--- Test 9: Sync Network Time (restore from NTP) ---\n");
    printf("Requesting time sync from network...\n");
    
    result = fn_clock_sync_network_time(&current_time);
    if (result != FN_OK) {
        printf("Failed to sync time: %s\n", fn_error_string(result));
        printf("(This may fail if network is not available)\n");
    } else {
        printf("Time synchronized from network.\n");
        printf("Current time:\n  ");
        print_time(&current_time);
    }
    printf("\n");
    
    printf("Done.\n");
    return 0;
}
