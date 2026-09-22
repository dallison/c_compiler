#ifndef davecc_TargetConditionals_h
#define davecc_TargetConditionals_h

#if defined(__aarch64__) || defined(__arm64__)
#define TARGET_CPU_ARM64 1
#define TARGET_CPU_X86_64 0
#elif defined(__x86_64__)
#define TARGET_CPU_ARM64 0
#define TARGET_CPU_X86_64 1
#else
#define TARGET_CPU_ARM64 0
#define TARGET_CPU_X86_64 0
#endif
#define TARGET_CPU_ARM 0
#define TARGET_CPU_X86 0
#define TARGET_CPU_PPC 0
#define TARGET_CPU_PPC64 0

#define TARGET_OS_MAC 1
#define TARGET_OS_OSX 1
#define TARGET_OS_IPHONE 0
#define TARGET_OS_IOS 0
#define TARGET_OS_TV 0
#define TARGET_OS_WATCH 0
#define TARGET_OS_VISION 0
#define TARGET_OS_MACCATALYST 0
#define TARGET_OS_SIMULATOR 0
#define TARGET_OS_EMBEDDED 0
#define TARGET_OS_UNIX 0
#define TARGET_OS_LINUX 0
#define TARGET_OS_WIN32 0
#define TARGET_OS_WINDOWS 0
#define TARGET_IPHONE_SIMULATOR 0
#define TARGET_OS_IPHONE_SIMULATOR 0

#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN 0
#define TARGET_RT_64_BIT 1
#define TARGET_RT_MAC_MACHO 1

#endif
