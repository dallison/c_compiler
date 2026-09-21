//
//  version.h
//  davecc
//

#ifndef davecc_version_h
#define davecc_version_h

#define DAVECC_VERSION_MAJOR 0
#define DAVECC_VERSION_MINOR 1

#define DAVECC_VERSION_STRINGIZE(x) #x
#define DAVECC_VERSION_STRINGIZE_VALUE(x) DAVECC_VERSION_STRINGIZE(x)
#define DAVECC_VERSION_STRING                   \
  DAVECC_VERSION_STRINGIZE_VALUE(DAVECC_VERSION_MAJOR) \
  "." DAVECC_VERSION_STRINGIZE_VALUE(DAVECC_VERSION_MINOR)

#define DAVECC_COPYRIGHT \
  "Copyright 2026 David Allison.  All Rights Reserved"

#ifdef __cplusplus
extern "C" {
#endif

// Current DaveCC version, e.g. "0.1".
const char* DaveCCVersion(void);

const char* DaveCCCopyright(void);

#ifdef __cplusplus
}
#endif

#endif
