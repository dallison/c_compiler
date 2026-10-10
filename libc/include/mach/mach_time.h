#ifndef _MACH_MACH_TIME_H_
#define _MACH_MACH_TIME_H_

#include <stdint.h>

struct mach_timebase_info {
  uint32_t numer;
  uint32_t denom;
};

typedef struct mach_timebase_info mach_timebase_info_data_t;
typedef struct mach_timebase_info* mach_timebase_info_t;

typedef int kern_return_t;

#ifdef __cplusplus
extern "C" {
#endif

kern_return_t mach_timebase_info(mach_timebase_info_t info);
uint64_t mach_absolute_time(void);

#ifdef __cplusplus
}
#endif

#endif /* _MACH_MACH_TIME_H_ */
