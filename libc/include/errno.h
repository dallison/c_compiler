//
//  errno.h
//  c_compiler
//
//  Created by David Allison on 1/19/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef errno_h
#define errno_h
#ifdef __DAVECC__

#define EDOM 200
#define EILSEQ 201
#define ERANGE 202

extern __thread int errno;

#endif /* __DAVECC__ */
#endif /* errno_h */
