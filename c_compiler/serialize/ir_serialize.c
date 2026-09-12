//
//  ir_serialize.c
//  c_compiler
//
//  Serialization of IR nodes for DCCLTO03 bitcode LTO modules.
//

#include <stdlib.h>
#include <string.h>

#include "ir.h"
#include "serialize_common.h"
#include "symbol.h"
#include "type.h"
#include "type_core.h"

enum {
  kIR_opcode = 1,
  kIR_flags = 2,
  kIR_type = 3,
  kIR_data_ivalue = 4,
  kIR_data_lvalue = 5,
  kIR_var_def = 6,
  kIR_var_use = 7,
  kIR_dest = 8,
  kIR_inputs = 9,
  kIR_location = 10,
  kIR_const_ivalue = 11,
  kIR_const_fvalue = 12,
  kIR_named_label = 13,
  kIR_variable_symbol = 14,
  kIR_value_state = 15,
  kIR_id = 16,
};

static const WireFieldDesc kIRFields[] = {
    {kIR_opcode, "opcode"},
    {kIR_flags, "flags"},
    {kIR_type, "type"},
    {kIR_data_ivalue, "data_ivalue"},
    {kIR_data_lvalue, "data_lvalue"},
    {kIR_var_def, "var_def"},
    {kIR_var_use, "var_use"},
    {kIR_dest, "dest"},
    {kIR_inputs, "inputs"},
    {kIR_location, "location"},
    {kIR_const_ivalue, "const_ivalue"},
    {kIR_const_fvalue, "const_fvalue"},
    {kIR_named_label, "named_label"},
    {kIR_variable_symbol, "variable_symbol"},
    {kIR_value_state, "value_state"},
    {kIR_id, "id"},
};

static bool IROpcodeIsConstant(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(const8):
    case IR_OP(const16):
    case IR_OP(const32):
    case IR_OP(const64):
    case IR_OP(constf):
    case IR_OP(constd):
    case IR_OP(consta):
      return true;
    default:
      return false;
  }
}

static bool IROpcodeIsFloatConstant(IROpcode opcode) {
  return opcode == IR_OP(constf) || opcode == IR_OP(constd);
}

static bool IROpcodeIsVariable(IROpcode opcode) {
  switch (opcode) {
    case IR_OP(localvar):
    case IR_OP(externvar):
    case IR_OP(argument):
    case IR_OP(staticvar):
    case IR_OP(tempvar):
    case IR_OP(ssavar):
    case IR_OP(phi):
      return true;
    default:
      return false;
  }
}

static size_t IRAllocSizeForOpcode(IROpcode opcode) {
  if (IROpcodeIsConstant(opcode)) {
    return sizeof(IRConstant);
  }
  if (opcode == IR_OP(named_label)) {
    return sizeof(IRNamedLabel);
  }
  if (opcode == IR_OP(loc)) {
    return sizeof(IRLocation);
  }
  if (IROpcodeIsVariable(opcode)) {
    return sizeof(IRVariable);
  }
  return sizeof(IRNode);
}

static bool CanInternIR(const void* obj) { return obj != NULL; }

static bool WriteIR(SerializeContext* ctx, WireBuffer* buf, void* obj) {
  IRNode* node = (IRNode*)obj;
  WireWriteInt32(buf, kIR_opcode, (int32_t)node->opcode);
  WireWriteInt32(buf, kIR_flags, node->flags);
  SWriteRef(ctx, buf, kIR_type, kSerialKindType, node->type);
  WireWriteInt32(buf, kIR_data_ivalue, node->data.ivalue);
  WireWriteInt64(buf, kIR_data_lvalue, node->data.lvalue);
  if ((node->flags & kIRVarDef) != 0) {
    SWriteRef(ctx, buf, kIR_var_def, kSerialKindSymbol, node->var.def);
  }
  if ((node->flags & kIRVarUse) != 0) {
    SWriteRef(ctx, buf, kIR_var_use, kSerialKindSymbol, node->var.use);
  }
  SWriteRef(ctx, buf, kIR_dest, kSerialKindIRNode, node->dest);
  SWriteRefVector(ctx, buf, kIR_inputs, kSerialKindIRNode, &node->inputs);
  WireWriteUint64(buf, kIR_location, node->location);
  WireWriteInt32(buf, kIR_value_state, (int32_t)node->value_state);
  WireWriteInt32(buf, kIR_id, node->id);
  if (IROpcodeIsConstant(node->opcode)) {
    IRConstant* c = (IRConstant*)node;
    if (IROpcodeIsFloatConstant(node->opcode)) {
      WireWriteDouble(buf, kIR_const_fvalue, c->value.fvalue);
    } else {
      WireWriteInt64(buf, kIR_const_ivalue, c->value.ivalue);
    }
  }
  if (node->opcode == IR_OP(named_label)) {
    IRNamedLabel* named = (IRNamedLabel*)node;
    if (named->name != NULL) {
      SerialHandle h = SerializeInternStringN(ctx, named->name, strlen(named->name));
      WireWriteVarint(buf, kIR_named_label, h);
    }
  }
  if (IROpcodeIsVariable(node->opcode)) {
    SWriteRef(ctx, buf, kIR_variable_symbol, kSerialKindSymbol,
              ((IRVariable*)node)->symbol);
  }
  return !WireBufferHasError(buf);
}

static void* AllocIR(DeserializeContext* ctx, const void* blob, size_t len) {
  (void)ctx;
  WireBuffer in;
  WireBufferInitReader(&in, blob, len);
  int32_t opcode = 0;
  bool have_op = false;
  while (!WireBufferEof(&in) && !WireBufferHasError(&in) && !have_op) {
    int field;
    WireType wt;
    if (!WireReadTag(&in, &field, &wt)) {
      break;
    }
    if (field == kIR_opcode) {
      WireReadInt32(&in, &opcode);
      have_op = true;
    } else {
      WireSkip(&in, wt);
    }
  }
  IROpcode op = (IROpcode)opcode;
  IRNode* node = calloc(1, IRAllocSizeForOpcode(op));
  IRInit(node, op);
  return node;
}

static bool ReadIR(DeserializeContext* ctx, WireBuffer* buf, void* obj) {
  IRNode* node = (IRNode*)obj;
  while (!WireBufferEof(buf) && !WireBufferHasError(buf)) {
    int field;
    WireType wt;
    if (!WireReadTag(buf, &field, &wt)) {
      break;
    }
    switch (field) {
      case kIR_opcode: {
        int32_t opcode = 0;
        WireReadInt32(buf, &opcode);
        node->opcode = (IROpcode)opcode;
        break;
      }
      case kIR_flags:
        WireReadInt32(buf, &node->flags);
        break;
      case kIR_type: {
        TypeRecord* type = (TypeRecord*)SReadRef(ctx, buf, kSerialKindType);
        if (type != NULL) {
          IRSetType(node, type);
        }
        break;
      }
      case kIR_data_ivalue:
        WireReadInt32(buf, &node->data.ivalue);
        break;
      case kIR_data_lvalue:
        WireReadInt64(buf, &node->data.lvalue);
        break;
      case kIR_var_def:
        node->var.def = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kIR_var_use:
        node->var.use = (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      case kIR_dest:
        node->dest = (IRNode*)SReadRef(ctx, buf, kSerialKindIRNode);
        break;
      case kIR_inputs:
        SReadRefVector(ctx, buf, kSerialKindIRNode, &node->inputs);
        for (size_t i = 0; i < node->inputs.length; i++) {
          IRNode* input = (IRNode*)node->inputs.value.p[i];
          if (input != NULL) {
            VectorAppend(&input->outputs, node);
          }
        }
        break;
      case kIR_location:
        WireReadUint64(buf, &node->location);
        break;
      case kIR_value_state: {
        int32_t vs = 0;
        WireReadInt32(buf, &vs);
        node->value_state = (ValueState)vs;
        break;
      }
      case kIR_id:
        WireReadInt32(buf, &node->id);
        break;
      case kIR_const_ivalue:
        if (IROpcodeIsConstant(node->opcode)) {
          WireReadInt64(buf, &((IRConstant*)node)->value.ivalue);
        } else {
          WireSkip(buf, wt);
        }
        break;
      case kIR_const_fvalue:
        if (IROpcodeIsConstant(node->opcode)) {
          WireReadDouble(buf, &((IRConstant*)node)->value.fvalue);
        } else {
          WireSkip(buf, wt);
        }
        break;
      case kIR_named_label: {
        uint64_t handle = 0;
        WireReadVarint(buf, &handle);
        size_t len = 0;
        const char* str =
            DeserializeResolveString(ctx, (SerialHandle)handle, &len);
        if (str != NULL) {
          ((IRNamedLabel*)node)->name = strdup(str);
        }
        break;
      }
      case kIR_variable_symbol:
        ((IRVariable*)node)->symbol =
            (Symbol*)SReadRef(ctx, buf, kSerialKindSymbol);
        break;
      default:
        WireSkip(buf, wt);
        break;
    }
  }
  if (node->opcode == IR_OP(loc)) {
    ((IRLocation*)node)->location = node->location;
  }
  return !WireBufferHasError(buf);
}

void SerializeRegisterIRKinds(void) {
  static const SerialKindVtable ir_vt = {WriteIR, AllocIR, ReadIR, "IR",
                                         CanInternIR};
  SerializeRegisterKind(kSerialKindIRNode, &ir_vt);
  SerializeRegisterFields(kSerialKindIRNode, kIRFields,
                          sizeof(kIRFields) / sizeof(kIRFields[0]));
}
