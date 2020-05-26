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
#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined
#endif

#endif /* __DAVECC__ */
#endif /* stdbool_h */
