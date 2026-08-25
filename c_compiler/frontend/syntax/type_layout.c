//
//  type_layout.c
//  c_compiler
//
#include "type_class_internal.h"
#include "type_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <assert.h>
#include "concepts.h"
#include "constexpr.h"
#include "dstring.h"
#include "expr_evaluator.h"
#include "expr_parser.h"
#include "expr_semantics.h"
#include "statement_semantics.h"
#include "statement_parser.h"
#include "symbol_table.h"
#include "syntax.h"
#include "semantics.h"
#include "compiler.h"
#include "errors.h"
#include "debug.h"
#include "rtti.h"
#include "set.h"


static int EffectiveMemberAlignment(Struct* str, TypeRecord* type,
                                    int explicit_alignment) {
  // A packed struct places members on byte boundaries with no padding, so the
  // effective member alignment is 1.
  int alignment = str->packed ? 1 : TypeRecordAlignment(type);
  if (!str->packed && explicit_alignment > alignment) {
    alignment = explicit_alignment;
  }
  // #pragma pack(n) caps the effective alignment of each member at n bytes.
  if (str->pack > 0 && alignment > str->pack) {
    alignment = str->pack;
  }
  return alignment > 0 ? alignment : 1;
}

static void AlignNextOffsetWithAlignment(Struct* str, TypeRecord* type,
                                         int explicit_alignment) {
  int alignment = EffectiveMemberAlignment(str, type, explicit_alignment);
  if (alignment > str->alignment) {
    str->alignment = alignment;
  }
  str->next_offset = (str->next_offset + (alignment - 1)) & ~(alignment - 1);
  str->next_bit_pos = 65;
  str->current_offset = str->next_offset;
}

void AlignNextOffset(Struct* str, TypeRecord* type) {
  AlignNextOffsetWithAlignment(str, type, 0);
}

void AlignNextOffsetForSymbol(Struct* str, Symbol* symbol) {
  AlignNextOffsetWithAlignment(str, symbol->type, symbol->alignment);
}

void FinalizeStructAlignment(Struct* str) {
  int align = str->alignment > 0 ? str->alignment : 1;
  if (CompilerIsCXX() && !str->is_union && str->size == 0) {
    str->size = 1;
    str->alignment = align;
  }
  if (str->explicit_alignment > align) {
    align = str->explicit_alignment;
    str->alignment = align;
  }
  str->size = (str->size + (align - 1)) & ~(align - 1);
  TypeRecordSyncStructSizes(str);
}

void StructApplyLayoutAttributes(Struct* str, Vector* attrs) {
  if (AttributeListHas(attrs, "packed")) {
    str->packed = true;
  }
  Attribute* aligned = AttributeListFind(attrs, "aligned");
  if (aligned != NULL) {
    long n = 0;
    int a;
    if (AttributeArgInt(aligned, 0, &n) && n > 0) {
      a = (int)n;
    } else {
      // aligned with no argument requests the target's maximum alignment.
      a = compiler->alignment;
    }
    if (a > str->explicit_alignment) {
      str->explicit_alignment = a;
    }
  }
}

void ParseBitField(TypeParser* parser, bool is_union, Struct* str,
                          Symbol* member_symbol, StructMember* member) {
  char error[256];
  ASTNode* width_node =
      SyntaxParseConditionalExpression(parser->syntax, TC(semicolon));
  if (width_node == NULL) {
    snprintf(error, sizeof(error), "constant expression needed");
    goto error;
  }
  int64_t bit_width = 0;
  if (!EvaluateIntegerExpression(width_node, &bit_width)) {
    ASTNodeDelete(width_node);
    snprintf(error, sizeof(error), "constant expression needed");
    goto error;
  }
  ASTNodeDelete(width_node);
  if (!TypeIsIntegral(member_symbol->type)) {
    snprintf(error, sizeof(error),
             "only integer types can be used for bitfields");
    goto error;
  }
  int word_width = TypeIsBitInt(member_symbol->type)
                       ? member_symbol->type->bit_width
                       : member_symbol->type->size * 8;
  bool unnamed = member_symbol->flags.invented;
  if (bit_width == 0) {
    if (!unnamed) {
      snprintf(error, sizeof(error), "named bit-field has zero width");
      goto error;
    }
    // Unnamed zero-width bit-fields are padding: the next bit-field starts at
    // a fresh allocation unit.  They are not an error.
    member->is_bit_field = true;
    member->bit_size = 0;
    member->bit_offset = 0;
    member->byte_offset = str->current_offset;
    str->next_bit_pos = word_width;
    return;
  }
  if (bit_width < 0 || bit_width > word_width) {
    snprintf(error, sizeof(error),
             "width of %" PRId64 " is out of bounds for type of size %d bits",
             bit_width, word_width);
    goto error;
  }
  int width = (int)bit_width;
  if (str->next_bit_pos + width > word_width) {
    // No room in current word for bit field (or first bit field).  We align
    // to the next boundary based on the bitfield type and add a new word
    // (of the appropriate type) to the struct.
    AlignNextOffsetForSymbol(
        str, member_symbol);  // Will set current_offset and next_offset.
    member->byte_offset = str->next_offset;
    member->index = str->members.length - 1;
    str->next_bit_pos = 0;
    if (!is_union) {
      str->next_offset += member_symbol->type->size;
      str->size = str->next_offset;
    } else {
      if (member_symbol->type->size > str->size) {
        str->size = member_symbol->type->size;
      }
    }
  } else {
    // There is room in the current word for the bitfield.
    member->byte_offset = str->current_offset;
  }
  member->is_bit_field = true;
  member->bit_size = width;
  member->bit_offset = str->next_bit_pos;
  if (!is_union) {
    str->next_bit_pos += width;
  }
  return;

error:
  // Diagnose and recover: keep the member as a zero-width bit-field so later
  // layout and member parsing do not dereference an incomplete field.
  SyntaxError(parser->syntax, "Invalid bitfield; %s", error);
  member->is_bit_field = true;
  member->bit_size = 0;
  member->bit_offset = 0;
  member->byte_offset = str->current_offset;
}

void UpdateStructSize(Struct* str, TypeRecord* member_type, bool is_union) {
   // Update the struct offset and size based on the
   // anonymous member.
   if (!is_union) {
     str->next_offset += member_type->size;
     str->size = str->next_offset;
   } else {
     // The size of a union is the maximum size of its members.
     if (member_type->size > str->size) {
       str->size = member_type->size;
     }
   }
}

static bool TypeIsEmptyClassForNoUniqueAddress(TypeRecord* type) {
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL || type->info.struct_info->is_union) {
    return false;
  }
  Struct* nested = type->info.struct_info;
  if (nested->bases.length > 0 || nested->virtual_bases.length > 0 ||
      nested->virtual_members.length > 0) {
    return false;
  }
  for (size_t i = 0; i < nested->members.length; i++) {
    StructMember* member = nested->members.value.p[i];
    if (member != NULL && !member->is_static && !member->is_member_function &&
        !member->is_using_declaration && !StructMemberIsNestedType(member)) {
      return false;
    }
  }
  return true;
}

static bool TryPlaceNoUniqueAddressMember(Struct* str,
                                          StructMember* member) {
  if (str == NULL || member == NULL || member->symbol == NULL ||
      str->is_union ||
      !AttributeListHas(&member->symbol->attributes, "no_unique_address") ||
      !TypeIsEmptyClassForNoUniqueAddress(member->symbol->type)) {
    return false;
  }
  int alignment = TypeRecordAlignment(member->symbol->type);
  if (alignment <= 0) {
    alignment = 1;
  }
  int occupied_size = str->size > 0 ? str->size : 1;
  for (int candidate = 0; candidate < occupied_size;
       candidate += alignment) {
    bool same_type_at_offset = false;
    for (size_t i = 0; i < str->members.length; i++) {
      StructMember* existing = str->members.value.p[i];
      if (existing == member) {
        break;
      }
      if (existing != NULL && existing->symbol != NULL &&
          existing->byte_offset == candidate &&
          TypeEqual(existing->symbol->type, member->symbol->type)) {
        same_type_at_offset = true;
        break;
      }
    }
    if (!same_type_at_offset) {
      member->byte_offset = candidate;
      return true;
    }
  }

  // Distinct potentially-overlapping subobjects of the same type still need
  // distinct addresses. If every existing byte is occupied by that type,
  // append this subobject normally.
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* existing = str->members.value.p[i];
    if (existing == member) {
      break;
    }
    if (existing != NULL && existing->symbol != NULL &&
        TypeEqual(existing->symbol->type, member->symbol->type)) {
      int after_existing =
          existing->byte_offset + existing->symbol->type->size;
      if (after_existing > str->next_offset) {
        str->next_offset = after_existing;
      }
    }
  }
  AlignNextOffsetForSymbol(str, member->symbol);
  member->byte_offset = str->next_offset;
  return false;
}

void StructAddSyntheticMember(Struct* str, StructMember* member) {
  bool overlaps = false;
  if (member->is_member_function) {
    SymbolSetCXXMangledAsmName(member->symbol);
  } else if (!member->is_static) {
    AlignNextOffset(str, member->symbol->type);
    member->byte_offset = str->next_offset;
    overlaps = TryPlaceNoUniqueAddressMember(str, member);
  }
  member->index = str->members.length;
  VectorAppend(&str->members, member);
  StructInsertMemberIntoTables(str, member);
  if (!member->is_static && !member->is_member_function && !overlaps) {
    UpdateStructSize(str, member->symbol->type, str->is_union);
  }
}

bool RelayoutStruct(Struct* str) {
  bool is_union = str->is_union;
  str->next_offset = 0;
  str->current_offset = 0;
  str->size = 0;
  str->non_virtual_size = 0;
  str->alignment = 1;
  str->next_bit_pos = 65;
  LayoutCXXBaseSpecifiers(str);
  for (size_t i = 0; i < str->members.length; i++) {
    StructMember* m = str->members.value.p[i];
    if (m->is_static || m->is_member_function || m->is_using_declaration ||
        StructMemberIsNestedType(m)) {
      continue;
    }
    TypeRecord* type = m->symbol != NULL ? m->symbol->type : NULL;
    if (type == NULL) {
      continue;
    }
    if (StructMemberIsBitField(m)) {
      int word_width = TypeIsBitInt(type) ? type->bit_width : type->size * 8;
      if (word_width <= 0) {
        continue;
      }
      if (m->bit_size <= 0) {
        // Zero-width padding (or a recovered invalid bit-field): the next
        // bit-field starts a new allocation unit.
        str->next_bit_pos = word_width;
        continue;
      }
      // Bitfield: replicate ParseBitField's placement using the stored width.
      if (str->next_bit_pos + m->bit_size > word_width) {
        AlignNextOffsetForSymbol(str, m->symbol);
        m->byte_offset = str->next_offset;
        m->index = i;
        str->next_bit_pos = 0;
        if (!is_union) {
          str->next_offset += type->size;
          str->size = str->next_offset;
        } else if (type->size > str->size) {
          str->size = type->size;
        }
      } else {
        m->byte_offset = str->current_offset;
      }
      m->bit_offset = str->next_bit_pos;
      if (!is_union) {
        str->next_bit_pos += m->bit_size;
      }
    } else {
      AlignNextOffsetForSymbol(str, m->symbol);
      m->byte_offset = str->next_offset;
      m->index = i;
      if (!TryPlaceNoUniqueAddressMember(str, m)) {
        UpdateStructSize(str, type, is_union);
      }
    }
  }
  str->non_virtual_size = str->size;
  LayoutCXXVirtualBaseSpecifiers(str);
  FinalizeStructAlignment(str);
  return true;
}

void TypeApplyStructAttributesFromSymbol(Symbol* sym) {
  if (sym == NULL || sym->attributes.length == 0) {
    return;
  }
  if (!AttributeListHas(&sym->attributes, "packed") &&
      AttributeListFind(&sym->attributes, "aligned") == NULL) {
    return;
  }
  TypeRecord* type = sym->type;
  if (type == NULL || !TypeIsStructOrUnion(type) ||
      type->info.struct_info == NULL) {
    return;
  }
  Struct* str = type->info.struct_info;
  bool was_packed = str->packed;
  int was_align = str->explicit_alignment;
  StructApplyLayoutAttributes(str, &sym->attributes);
  if (str->packed != was_packed || str->explicit_alignment != was_align) {
    if (RelayoutStruct(str)) {
      // The type's size may already have been cached at the old (unpacked)
      // value; force it to be recomputed from the struct's new size.
      type->size = 0;
      TypeRecordCalculateSize(type);
    }
  }
}
