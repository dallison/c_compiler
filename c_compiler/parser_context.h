//
//  parser_context.h
//  c_compiler
//
//  Created by David Allison on 7/20/20.
//  Copyright © 2020 David Allison. All rights reserved.
//

#ifndef parser_context_h
#define parser_context_h

typedef enum ParserContext {
  kParsingFileScope,
  kParsingBlockScope,
  kParsingPrototype,
  kParsingStructOrUnion,
} ParserContext;

#endif /* parser_context_h */
