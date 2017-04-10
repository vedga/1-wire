#if !defined(__PLATFORM_H)
#define __PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

typedef bool bool_t;

#include <intrinsics.h>
#include <iostm8l152c6.h>

#define __INLINE inline
#define __EXTERN extern


#define array_size(_n)                                                  \
  (sizeof(_n)/sizeof(_n[0]))



#endif
