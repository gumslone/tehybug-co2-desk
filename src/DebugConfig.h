
#pragma once

/**
 * @brief Global debug configuration
 *
 * Set to false to disable debug output in your code.
 * Can be overridden at build time, e.g.
 *   --build-property "compiler.cpp.extra_flags=-DDEBUG_ENABLED=1"
 * Note: This doesn't affect library debug output.
 */
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED false
#endif

// Helper macros for conditional debug output
#if DEBUG_ENABLED
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
    #define DEBUG_PRINT_JSON(doc) serializeJsonPretty(doc, Serial)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(...)
    #define DEBUG_PRINT_JSON(doc)
#endif