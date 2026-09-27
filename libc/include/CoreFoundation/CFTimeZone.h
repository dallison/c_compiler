// Minimal CoreFoundation timezone declarations for DaveCC.  The Apple SDK
// headers pull in attributes this compiler does not parse.  The functions
// below are provided by libSystem.
#ifndef __DAVECC_CFTIMEZONE_H__
#define __DAVECC_CFTIMEZONE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef signed long CFIndex;
typedef unsigned int CFStringEncoding;
typedef unsigned char Boolean;
typedef const void* CFTypeRef;
typedef const struct __CFString* CFStringRef;
typedef struct __CFTimeZone* CFTimeZoneRef;

enum { kCFStringEncodingUTF8 = 0x08000100 };

CFTimeZoneRef CFTimeZoneCopyDefault(void);
CFStringRef CFTimeZoneGetName(CFTimeZoneRef tz);
CFIndex CFStringGetLength(CFStringRef str);
CFIndex CFStringGetMaximumSizeForEncoding(CFIndex length,
                                          CFStringEncoding encoding);
Boolean CFStringGetCString(CFStringRef str, char* buffer, CFIndex buffer_size,
                           CFStringEncoding encoding);
void CFRelease(CFTypeRef cf);

#ifdef __cplusplus
}
#endif

#endif
