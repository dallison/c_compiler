//
//  wasm32_emitter.c
//  c_compiler
//

#include "wasm32_emitter.h"

#include "wasm32_reg_alloc.h"

static const char* TypeName(WasmValueType type) {
  switch (type) {
    case kWasmTypeI64:
      return "i64";
    case kWasmTypeF32:
      return "f32";
    case kWasmTypeF64:
      return "f64";
    case kWasmTypeI32:
      return "i32";
    default:
      return "void";
  }
}

static void PrintSignature(Wasm32Generator* wasm, FILE* fp) {
  fprintf(fp, "(func $%s", wasm->base.function_name.value);
  for (size_t i = 0; i < wasm->signature.param_types.length; i++) {
    intptr_t type = (intptr_t)wasm->signature.param_types.value.p[i];
    fprintf(fp, " (param %s)", TypeName((WasmValueType)type));
  }
  if (wasm->signature.has_result) {
    fprintf(fp, " (result %s)", TypeName(wasm->signature.result_type));
  }
  fprintf(fp, "\n");
}

static void PrintLocals(Wasm32Generator* wasm, FILE* fp) {
  static const WasmValueType kTypeOrder[4] = {kWasmTypeI32, kWasmTypeI64,
                                              kWasmTypeF32, kWasmTypeF64};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < wasm->num_locals[i]; j++) {
      fprintf(fp, "  (local %s)\n", TypeName(kTypeOrder[i]));
    }
  }
}

static void PrintOperands(TargetInstruction* inst, FILE* fp) {
  for (int i = 0; i < TARGET_MAX_OPERANDS; i++) {
    TargetInstruction* operand = inst->operand[i];
    if (operand == NULL || Wasm32OperandIsImmediate(inst, i)) {
      continue;
    }
    fprintf(fp, "  local.get %d\n", Wasm32LocalIndex(operand));
  }
}

void Wasm32PrintFunction(Wasm32Generator* wasm, FILE* fp) {
  PrintSignature(wasm, fp);
  PrintLocals(wasm, fp);

  TargetInstruction* inst = TargetFirstInstruction(&wasm->base);
  while (inst != NULL) {
    if ((inst->flags & TARGET_INST_DEAD) != 0 || Wasm32IsPseudo(inst)) {
      inst = TargetNext(inst);
      continue;
    }
    Wasm32Opcode opcode = (Wasm32Opcode)inst->opcode;
    switch (opcode) {
      case W_OP(label):
        fprintf(fp, ";; label %d\n", inst->id);
        break;
      case W_OP(named_label):
        fprintf(fp, ";; label %s\n", ((TargetNamedLabel*)inst)->name);
        break;
      case W_OP(mov):
      case W_OP(movf):
      case W_OP(movd):
        fprintf(fp, "  local.get %d\n",
                Wasm32LocalIndex(inst->operand[0]));
        fprintf(fp, "  local.set %d\n",
                inst->dest != NULL ? Wasm32LocalIndex(inst->dest)
                                   : Wasm32LocalIndex(inst));
        break;
      case W_OP(i32_const):
      case W_OP(i64_const):
        PrintOperands(inst, fp);
        fprintf(fp, "  %s %lld\n", Wasm32OpcodeName(opcode),
                (long long)((TargetConstant*)inst->operand[0])->value.ivalue);
        fprintf(fp, "  local.set %d\n", Wasm32LocalIndex(inst));
        break;
      case W_OP(f32_const):
      case W_OP(f64_const):
        fprintf(fp, "  %s %f\n", Wasm32OpcodeName(opcode),
                ((TargetConstant*)inst->operand[0])->value.dvalue);
        fprintf(fp, "  local.set %d\n", Wasm32LocalIndex(inst));
        break;
      case W_OP(br):
      case W_OP(br_if):
        PrintOperands(inst, fp);
        fprintf(fp, "  %s %d\n", Wasm32OpcodeName(opcode),
                inst->addr < 0 ? 0 : inst->addr);
        break;
      default:
        PrintOperands(inst, fp);
        fprintf(fp, "  %s\n", Wasm32OpcodeName(opcode));
        if (Wasm32ProducesValue(inst)) {
          fprintf(fp, "  local.set %d\n",
                  inst->dest != NULL ? Wasm32LocalIndex(inst->dest)
                                     : Wasm32LocalIndex(inst));
        }
        break;
    }
    inst = TargetNext(inst);
  }
  if (wasm->signature.has_result) {
    fprintf(fp, "  unreachable\n");
  }
  fprintf(fp, ")\n\n");
}
