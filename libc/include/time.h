//
//  time.h
//  c_compiler
//
//  Created by David Allison on 5/11/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef time_h
#define time_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __SIZE_T
#if defined(__W65C02__)
typedef unsigned int size_t;
#else
typedef unsigned long size_t;
#endif
#define __SIZE_T
#endif

#ifndef __SSIZE_T
#if defined(__W65C02__)
typedef int ssize_t;
#else
typedef long ssize_t;
#endif
#define __SSIZE_T
#endif

#ifndef __OFF_T
#define __OFF_T
typedef unsigned long off_t;
#endif

typedef long time_t;
typedef long clock_t;

struct timespec {
   time_t tv_sec;
   long tv_nsec;
};

extern time_t   time(time_t *);

struct tm {
   int     tm_sec;         /* seconds */
   int     tm_min;         /* minutes */
   int     tm_hour;        /* hours */
   int     tm_mday;        /* day of the month */
   int     tm_mon;         /* month */
   int     tm_year;        /* year */
   int     tm_wday;        /* day of the week */
   int     tm_yday;        /* day in the year */
   int     tm_isdst;       /* daylight saving time */

   long int tm_gmtoff;     /* Seconds east of UTC.  */
   const char *tm_zone;    /* Timezone abbreviation.  */

};

/* defining TM_ZONE indicates that we have a "timezone abbreviation" field in
 * struct tm, the value should be the field name
 */
#define   TM_ZONE   tm_zone

extern char* asctime(const struct tm* a);
extern char* asctime_r(const struct tm* a, char* buf);

/* Return the difference between TIME1 and TIME0.  */
extern double difftime (time_t __time1, time_t __time0);
extern time_t mktime (struct tm *a);

extern struct tm*  localtime(const time_t *t);
extern struct tm*  localtime_r(const time_t *timep, struct tm *result);

extern struct tm*  gmtime(const time_t *timep);
extern struct tm*  gmtime_r(const time_t *timep, struct tm *result);

extern char*       strptime(const char *buf, const char *fmt, struct tm *tm);
extern size_t      strftime(char *s, size_t max, const char *format, const struct tm *tm);

extern char *ctime(const time_t *timep);
extern char *ctime_r(const time_t *timep, char *buf);

extern void  tzset(void);

/* global includes */
extern char*     tzname[];

#define CLOCKS_PER_SEC     1000000

extern clock_t   clock(void);

#ifdef __cplusplus
}
#endif

#endif /* time_h */
