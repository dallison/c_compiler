#ifndef davecc_sys_auxv_h
#define davecc_sys_auxv_h

#define AT_HWCAP 16
#define AT_HWCAP2 26
#define AT_PAGESZ 6

#ifdef __cplusplus
extern "C" {
#endif

unsigned long getauxval(unsigned long type);

#ifdef __cplusplus
}
#endif

#endif
