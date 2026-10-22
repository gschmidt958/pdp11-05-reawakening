#ifndef BT201LOG_H
#define BT201LOG_H
#define BT201_ENABLE_DEBUG
/*
 * BT201Log
 *
 * Logging macros for the BT201 library.
 *
 * ERROR_print / ERROR_println  — always compiled in. Use for unexpected
 *                                conditions, protocol violations, queue
 *                                overflows, and dropped data.
 *
 * DEBUG_print / DEBUG_println  — compiled out unless BT201_ENABLE_DEBUG
 *                                is defined. Use for normal operational
 *                                flow tracing during development.
 *
 * To enable debug output, define BT201_ENABLE_DEBUG before including
 * any BT201 header, or add it as a compiler flag:
 *
 *   #define BT201_ENABLE_DEBUG
 */

#define ERROR_print(x)   Serial.print(x)
#define ERROR_println(x) Serial.println(x)

#ifdef BT201_ENABLE_DEBUG
  #define DEBUG_print(x)   Serial.print(x)
  #define DEBUG_println(x) Serial.println(x)
#else
  #define DEBUG_print(x)
  #define DEBUG_println(x)
#endif

#endif // BT201LOG_H