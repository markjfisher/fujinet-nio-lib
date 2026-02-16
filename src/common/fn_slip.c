/**
 * @file fn_slip.c
 * @brief SLIP Encoding/Decoding Implementation
 * 
 * Implements SLIP (Serial Line IP) framing for FujiBus packets.
 * SLIP provides simple packet delimiting over byte-stream transports.
 * 
 * @version 1.0.0
 */

#include "fn_protocol.h"

/**
 * Encode data with SLIP framing.
 * 
 * Adds SLIP END markers at start and end, and escapes any END or ESCAPE
 * bytes in the data.
 * 
 * @param input    Input data buffer
 * @param in_len   Length of input data
 * @param output   Output buffer for SLIP-encoded data
 * @return Length of encoded data
 */
uint16_t fn_slip_encode(const uint8_t *input, uint16_t in_len, uint8_t *output)
{
    uint16_t out_len;
    uint16_t i;
    uint8_t b;
    
    out_len = 0;
    
    /* Start with END marker */
    output[out_len++] = SLIP_END;
    
    /* Process each input byte */
    for (i = 0; i < in_len; i++) {
        b = input[i];
        
        if (b == SLIP_END) {
            /* Escape END byte */
            output[out_len++] = SLIP_ESCAPE;
            output[out_len++] = SLIP_ESC_END;
        } else if (b == SLIP_ESCAPE) {
            /* Escape ESCAPE byte */
            output[out_len++] = SLIP_ESCAPE;
            output[out_len++] = SLIP_ESC_ESC;
        } else {
            /* Normal byte */
            output[out_len++] = b;
        }
    }
    
    /* End with END marker */
    output[out_len++] = SLIP_END;
    
    return out_len;
}

/**
 * Decode SLIP-framed data.
 * 
 * Removes SLIP framing and un-escapes any escaped bytes.
 * 
 * @param input    Input SLIP-encoded data
 * @param in_len   Length of input data
 * @param output   Output buffer for decoded data
 * @return Length of decoded data, or 0 on error
 */
uint16_t fn_slip_decode(const uint8_t *input, uint16_t in_len, uint8_t *output)
{
    uint16_t out_len;
    uint16_t i;
    uint8_t b;
    
    out_len = 0;
    i = 0;
    
    /* Skip leading END marker if present */
    if (in_len > 0 && input[0] == SLIP_END) {
        i = 1;
    }
    
    /* Process until next END or end of data */
    while (i < in_len) {
        b = input[i++];
        
        if (b == SLIP_END) {
            /* End of packet */
            break;
        }
        
        if (b == SLIP_ESCAPE) {
            /* Escaped byte */
            if (i >= in_len) {
                /* Incomplete escape sequence */
                return 0;
            }
            
            b = input[i++];
            
            if (b == SLIP_ESC_END) {
                output[out_len++] = SLIP_END;
            } else if (b == SLIP_ESC_ESC) {
                output[out_len++] = SLIP_ESCAPE;
            } else {
                /* Invalid escape sequence */
                return 0;
            }
        } else {
            /* Normal byte */
            output[out_len++] = b;
        }
    }
    
    return out_len;
}

/**
 * Calculate the maximum encoded size for a given input size.
 * 
 * Worst case: every byte needs escaping (2x) plus 2 END markers.
 * 
 * @param in_len   Input data length
 * @return Maximum encoded length
 */
uint16_t fn_slip_max_encoded_size(uint16_t in_len)
{
    return (in_len * 2) + 2;
}
