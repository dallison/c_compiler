//
//  gvn.h
//  c_compiler
//
//  Created by David Allison on 12/20/17.
//  Copyright © 2017 David Allison. All rights reserved.
//

#ifndef gvn_h
#define gvn_h

#include <stdio.h>
#include "codegen.h"
#include "hashtable.h"
#include "vector.h"

// Global Value Numbering optimization.

// This is a value assigned to an expression.  In this case, we
// are dealing with IR nodes so an expression is an IRNode that
// generates a value (such as loadi, addi, muli, etc.).  These
// are held in a vector inside a hash table.
typedef struct {
  int64_t key;           // Unique key for value.
  int32_t value_number;  // The value number assigned.
  IRNode* instruction;   // IR instruction represented by this value.
} Value;

Value* NewValue(int64_t key, int32_t value_number, IRNode* inst);
void ValueDelete(Value* value);
Value* ValueCopy(Value* v);

// A ValueSet is a set of values, represented as a hash table of vectors
// containing pointers to Value structs.
typedef struct {
  // The value table.  This is a hash table of pointers to
  // Vector of Value structs.
  HashTable values;
  int32_t next_value_number;
} ValueSet;

void ValueInitInit(ValueSet* opt);
ValueSet* NewValueSet(void);
void ValueSetDelete(ValueSet* opt);

// Main GVN function to perform the optimization.
void GlobalValueNumberingOptimization(Generator* gen);

#endif /* gvn_h */
