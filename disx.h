// disx.h

#ifndef _DISX_H_
#define _DISX_H_

// headers for everybody
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// define a type for addresses (could maybe also use ofs_t)
typedef long addr_t;

// =====================================================
// stuff to fix warnings

// disable unwanted warnings for xcode
#if defined(__clang__)
 #pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

// fall-through annotation to prevent warnings
#if defined(__clang__) && __cplusplus >= 201103L
 #define FALLTHROUGH [[clang::fallthrough]]
#elif defined(_MSC_VER)
 #include <sal.h>
 #define FALLTHROUGH __fallthrough
#elif defined(__GNUC__) && __GNUC__ >= 7
 #define FALLTHROUGH __attribute__ ((fallthrough))
#elif defined (__has_cpp_attribute)
 #if __has_cpp_attribute(fallthrough)
  #define FALLTHROUGH [[fallthrough]]
 #else // default version
  #define FALLTHROUGH ((void)0)
 #endif
#else // default version
 #define FALLTHROUGH ((void)0)
#endif /* __GNUC__ >= 7 */

// "void func(int UNUSED name)" for unused parameters
// this looks better than #define UNUSED(x) (void)(x)
#define UNUSED __attribute__((unused))

// =====================================================
// asserts support, should move NDEBUG to makefile later
#if 1 // custom assert, may not work on some environments
 #include "disassert.h"
#else
 #include <assert.h>
#endif
//#define NDEBUG


#endif // _DISX_H_
