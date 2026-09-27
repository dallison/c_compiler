//
//  stdbool.h
//  c_compiler
//
//  Created by David Allison on 1/19/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef stdbool_h
#define stdbool_h
#ifdef __DAVECC__

#ifndef __bool_true_false_are_defined
// C++ has bool, true, and false as keywords.  C23 does too.  Defining them
// as macros breaks declarations such as `extern "C" bool f(...);` and
// member templates written `constexpr bool operator()`.
#if !defined(__cplusplus) && \
    (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L)
#define bool _Bool
#define true 1
#define false 0
#endif
#define __bool_true_false_are_defined 1
#endif

#endif /* __DAVECC__ */
#endif /* stdbool_h */
