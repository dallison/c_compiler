#include "unicode_name.h"

#include <string.h>

#include "generated/unicode_name_data.h"

typedef struct {
  const char* segment;
  size_t segment_length;
  uint32_t value;
  uint32_t children_offset;
  size_t size;
  bool has_value;
  bool has_children;
  bool has_sibling;
} UnicodeNameNode;

static const char kSingleCharacters[] =
    " ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789";

static bool ReadUnicodeNameNode(uint32_t offset, UnicodeNameNode* node) {
  if (offset + 2 >= kUnicodeNameIndexSize) {
    return false;
  }
  uint32_t origin = offset;
  uint8_t descriptor = kUnicodeNameIndex[offset++];
  bool long_segment = (descriptor & 0x40) != 0;
  node->has_value = (descriptor & 0x80) != 0;
  descriptor &= 0x3f;
  if (long_segment) {
    if (descriptor == 0 || offset + 2 > kUnicodeNameIndexSize) {
      return false;
    }
    uint32_t dictionary_offset = (uint32_t)kUnicodeNameIndex[offset] << 8 |
                                 kUnicodeNameIndex[offset + 1];
    offset += 2;
    node->segment = &kUnicodeNameDictionary[dictionary_offset];
    node->segment_length = descriptor;
  } else {
    if (descriptor >= sizeof(kSingleCharacters) - 1) {
      return false;
    }
    node->segment = &kSingleCharacters[descriptor];
    node->segment_length = 1;
  }

  node->value = 0;
  node->children_offset = 0;
  if (node->has_value) {
    if (offset + 3 > kUnicodeNameIndexSize) {
      return false;
    }
    uint32_t packed = (uint32_t)kUnicodeNameIndex[offset] << 16 |
                      (uint32_t)kUnicodeNameIndex[offset + 1] << 8 |
                      kUnicodeNameIndex[offset + 2];
    offset += 3;
    node->value = packed >> 3;
    node->has_sibling = (packed & 0x1) != 0;
    node->has_children = (packed & 0x2) != 0;
    if (node->has_children) {
      if (offset + 3 > kUnicodeNameIndexSize) {
        return false;
      }
      node->children_offset =
          (uint32_t)kUnicodeNameIndex[offset] << 16 |
          (uint32_t)kUnicodeNameIndex[offset + 1] << 8 |
          kUnicodeNameIndex[offset + 2];
      offset += 3;
    }
  } else {
    uint8_t flags = kUnicodeNameIndex[offset++];
    node->has_sibling = (flags & 0x80) != 0;
    node->has_children = (flags & 0x40) != 0;
    if (node->has_children) {
      if (offset + 2 > kUnicodeNameIndexSize) {
        return false;
      }
      node->children_offset =
          (uint32_t)(flags & 0x3f) << 16 |
          (uint32_t)kUnicodeNameIndex[offset] << 8 |
          kUnicodeNameIndex[offset + 1];
      offset += 2;
    }
  }
  node->size = offset - origin;
  return true;
}

static bool LookupUnicodeNameNodes(uint32_t offset, const char* name,
                                   size_t length, size_t position,
                                   uint32_t* codepoint) {
  for (;;) {
    UnicodeNameNode node;
    if (!ReadUnicodeNameNode(offset, &node)) {
      return false;
    }
    bool matches =
        node.segment_length <= length - position &&
        memcmp(&name[position], node.segment, node.segment_length) == 0;
    if (matches) {
      size_t next = position + node.segment_length;
      if (next == length && node.has_value) {
        *codepoint = node.value;
        return true;
      }
      if (next < length && node.has_children &&
          LookupUnicodeNameNodes(node.children_offset, name, length, next,
                                 codepoint)) {
        return true;
      }
    }
    if (!node.has_sibling) {
      return false;
    }
    offset += node.size;
  }
}

static size_t MatchLongest(const char* name, size_t length,
                           const char* const* values, size_t count,
                           int* matched) {
  size_t longest = 0;
  *matched = -1;
  for (size_t i = 0; i < count; i++) {
    size_t value_length = strlen(values[i]);
    if ((value_length > longest || (*matched == -1 && value_length == 0)) &&
        value_length <= length &&
        memcmp(name, values[i], value_length) == 0) {
      longest = value_length;
      *matched = (int)i;
    }
  }
  return longest;
}

static bool LookupHangulSyllable(const char* name, size_t length,
                                 uint32_t* codepoint) {
  static const char prefix[] = "HANGUL SYLLABLE ";
  static const char* const leading[] = {
      "G", "GG", "N", "D", "DD", "R", "M", "B", "BB", "S",
      "SS", "",   "J", "JJ", "C", "K", "T", "P", "H",
  };
  static const char* const vowel[] = {
      "A",  "AE", "YA",  "YAE", "EO", "E",  "YEO",
      "YE", "O",  "WA",  "WAE", "OE", "YO", "U",
      "WEO", "WE", "WI", "YU",  "EU", "YI", "I",
  };
  static const char* const trailing[] = {
      "",   "G",  "GG", "GS", "N",  "NJ", "NH", "D", "L",  "LG",
      "LM", "LB", "LS", "LT", "LP", "LH", "M",  "B", "BS", "S",
      "SS", "NG", "J",  "C",  "K",  "T",  "P",  "H",
  };
  size_t prefix_length = sizeof(prefix) - 1;
  if (length < prefix_length ||
      memcmp(name, prefix, prefix_length) != 0) {
    return false;
  }
  name += prefix_length;
  length -= prefix_length;
  int l;
  int v;
  int t;
  size_t used = MatchLongest(name, length, leading,
                             sizeof(leading) / sizeof(leading[0]), &l);
  name += used;
  length -= used;
  used = MatchLongest(name, length, vowel, sizeof(vowel) / sizeof(vowel[0]),
                      &v);
  name += used;
  length -= used;
  used = MatchLongest(name, length, trailing,
                      sizeof(trailing) / sizeof(trailing[0]), &t);
  length -= used;
  if (l < 0 || v < 0 || t < 0 || length != 0) {
    return false;
  }
  *codepoint = 0xac00 + (uint32_t)((l * 21 + v) * 28 + t);
  return true;
}

static bool LookupAlgorithmicName(const char* name, size_t length,
                                  uint32_t* codepoint) {
  for (size_t i = 0; i < kUnicodeAlgorithmicNameRangeCount; i++) {
    const UnicodeAlgorithmicNameRange* range =
        &kUnicodeAlgorithmicNameRanges[i];
    size_t prefix_length = strlen(range->prefix);
    if (length <= prefix_length ||
        memcmp(name, range->prefix, prefix_length) != 0) {
      continue;
    }
    uint32_t value = 0;
    const char* digits = &name[prefix_length];
    size_t digit_count = length - prefix_length;
    if (digits[0] == '0') {
      continue;
    }
    bool valid = true;
    for (size_t j = 0; j < digit_count; j++) {
      unsigned char digit = (unsigned char)digits[j];
      uint32_t numeric;
      if (digit >= '0' && digit <= '9') {
        numeric = digit - '0';
      } else if (digit >= 'A' && digit <= 'F') {
        numeric = digit - 'A' + 10;
      } else {
        valid = false;
        break;
      }
      if (value > (UINT32_MAX - numeric) / 16) {
        valid = false;
        break;
      }
      value = value * 16 + numeric;
    }
    if (valid && value >= range->first && value <= range->last) {
      *codepoint = value;
      return true;
    }
  }
  return false;
}

bool UnicodeCodePointFromName(const char* name, size_t length,
                              uint32_t* codepoint) {
  if (name == NULL || codepoint == NULL || length == 0) {
    return false;
  }
  if (LookupHangulSyllable(name, length, codepoint) ||
      LookupAlgorithmicName(name, length, codepoint)) {
    return true;
  }
  return LookupUnicodeNameNodes(0, name, length, 0, codepoint);
}
