
#pragma once

/**
 * @brief Global debug configuration
 * 
 * Set to false to disable debug output in your code.
 * Note: This doesn't affect library debug output.
 */
#define DEBUG_ENABLED false

// Helper macros for conditional debug output
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
#endif