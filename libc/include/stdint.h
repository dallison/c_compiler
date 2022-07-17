//
//  stdint.h
//  c_compiler
//
//  Created by David Allison on 1/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stdint_h
#define stdint_h
#ifdef __DAVECC__


#if defined(__6502__)
// On 6502, pointers and ints are 16 bits long.
typedef char int8_t;
typedef int int16_t;
typedef long int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned int uintptr_t;
#else
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
typedef unsigned long uintptr_t;
#endif

#endif /* __DAVECC__ */
#endif /* stdint_h */
