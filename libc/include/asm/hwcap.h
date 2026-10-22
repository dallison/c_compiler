#ifndef davecc_asm_hwcap_h
#define davecc_asm_hwcap_h

#define HWCAP_FP (1u << 0)
#define HWCAP_ASIMD (1u << 1)
#define HWCAP_AES (1u << 3)
#define HWCAP_PMULL (1u << 4)
#define HWCAP_SHA1 (1u << 5)
#define HWCAP_SHA2 (1u << 6)
#define HWCAP_CRC32 (1u << 7)
#define HWCAP_ATOMICS (1u << 8)
#define HWCAP_CPUID (1u << 11)

#endif
