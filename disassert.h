// disassert.h

#ifndef _DISASSERT_H_
#define _DISASSERT_H_



#if 0
#include <assert.h>
#else

#ifdef NDEBUG
#define	assert(e)	((void)0)
#else // !NDEBUG

extern "C" void my_assert_func(const char *, const char *, int);

#define __assert(e, file, line) my_assert_func(e, file, line)
#define assert(e)  \
    ((void) ((e) ? ((void)0) : __assert (#e, __FILE__, __LINE__)))

#endif // !NDEBUG

#endif

#endif // _DISASSERT_H_
