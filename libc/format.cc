#include <format>

namespace std {
namespace __format_detail {

[[noreturn]] void __fail(const char* message) {
  __DAVECC_THROW(format_error(message));
}

void __append_repeat(string& output, char value, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    output.push_back(value);
  }
}

int __dynamic_integer(const __format_args* arguments, size_t index) {
  const __format_arg* arg = arguments != nullptr ? arguments->get(index) : nullptr;
  long long value = 0;
  if (arg == nullptr || arg->integer_value == nullptr ||
      !arg->integer_value(arg->value, &value) || value < 0) {
    __fail("invalid dynamic format argument");
  }
  return static_cast<int>(value);
}

static inline void __parse_spec_range(const string_view& text, size_t start,
                                      size_t size, const __format_args* arguments,
                                      __spec* result) {
  size_t position = start;
  size_t end = start + size;
  size_t next_dynamic = 0;
  if (position + 1 < end &&
      (text[position + 1] == '<' || text[position + 1] == '>' ||
       text[position + 1] == '^')) {
    result->fill = text[position];
    result->align = text[position + 1];
    position += 2;
  } else if (position < end &&
             (text[position] == '<' || text[position] == '>' ||
              text[position] == '^')) {
    result->align = text[position++];
  }
  if (position < end &&
      (text[position] == '+' || text[position] == '-' ||
       text[position] == ' ')) {
    result->sign = text[position++];
  }
  if (position < end && text[position] == '#') {
    result->alternate = true;
    ++position;
  }
  if (position < end && text[position] == '0') {
    result->zero = true;
    ++position;
  }
  if (position < end && __is_digit(text[position])) {
    result->width = 0;
    while (position < end && __is_digit(text[position])) {
      result->width = result->width * 10 + (text[position] - '0');
      ++position;
    }
  } else if (position < end && text[position] == '{') {
    ++position;
    size_t id = 0;
    bool explicit_id = false;
    while (position < end && __is_digit(text[position])) {
      explicit_id = true;
      id = id * 10 + static_cast<size_t>(text[position] - '0');
      ++position;
    }
    if (position >= end || text[position] != '}') {
      __fail("invalid dynamic format specification");
    }
    ++position;
    if (!explicit_id) {
      id = arguments != nullptr ? arguments->next_dynamic++ : next_dynamic++;
    }
    result->width = __dynamic_integer(arguments, id);
  }
  if (position < end && text[position] == '.') {
    ++position;
    if (position < end && __is_digit(text[position])) {
      result->precision = 0;
      while (position < end && __is_digit(text[position])) {
        result->precision =
            result->precision * 10 + (text[position] - '0');
        ++position;
      }
    } else if (position < end && text[position] == '{') {
      ++position;
      size_t id = 0;
      bool explicit_id = false;
      while (position < end && __is_digit(text[position])) {
        explicit_id = true;
        id = id * 10 + static_cast<size_t>(text[position] - '0');
        ++position;
      }
      if (position >= end || text[position] != '}') {
        __fail("invalid dynamic format specification");
      }
      ++position;
      if (!explicit_id) {
        id = arguments != nullptr ? arguments->next_dynamic++ : next_dynamic++;
      }
      result->precision = __dynamic_integer(arguments, id);
    } else {
      __fail("missing precision");
    }
  }
  if (position < end && text[position] == 'L') {
    result->localized = true;
    ++position;
  }
  if (position < end) {
    result->type = text[position++];
  }
  if (position != end) {
    __fail("invalid format specification");
  }
}

__spec __parse_spec(string_view text, const __format_args* arguments) {
  __spec result;
  __parse_spec_range(text, 0, text.size(), arguments, &result);
  return result;
}

size_t __prefix_length(const string& value) {
  size_t result = 0;
  if (!value.empty() &&
      (value[0] == '-' || value[0] == '+' || value[0] == ' ')) {
    result = 1;
  }
  if (result + 1 < value.size() && value[result] == '0' &&
      (value[result + 1] == 'x' || value[result + 1] == 'X' ||
       value[result + 1] == 'b' || value[result + 1] == 'B')) {
    result += 2;
  }
  return result;
}

static bool __utf8_decode(string_view value, size_t position,
                          unsigned long* code_point, size_t* length) {
  if (position >= value.size()) {
    return false;
  }
  unsigned long first =
      static_cast<unsigned long>(value[position]) & 0xFFUL;
  if (first < 0x80) {
    *code_point = first;
    *length = 1;
    return true;
  }
  size_t count = 0;
  if (first >= 0xF0 && first <= 0xF4) {
    count = 4;
  } else if (first >= 0xE0 && first <= 0xEF) {
    count = 3;
  } else if (first >= 0xC2 && first <= 0xDF) {
    count = 2;
  }
  if (count == 0 || position + count > value.size()) {
    *code_point = 0xFFFD;
    *length = 1;
    return false;
  }
  unsigned long result = first & (0x7FUL >> count);
  for (size_t i = 1; i < count; ++i) {
    unsigned long next =
        static_cast<unsigned long>(value[position + i]) & 0xFFUL;
    if ((next & 0xC0) != 0x80) {
      *code_point = 0xFFFD;
      *length = 1;
      return false;
    }
    result = (result << 6) | (next & 0x3F);
  }
  if ((count == 2 && result < 0x80) ||
      (count == 3 && result < 0x800) ||
      (count == 4 && (result < 0x10000 || result > 0x10FFFF)) ||
      (result >= 0xD800 && result <= 0xDFFF)) {
    *code_point = 0xFFFD;
    *length = 1;
    return false;
  }
  *code_point = result;
  *length = count;
  return true;
}

static bool __unicode_combining(unsigned long value) {
  return (value >= 0x0300 && value <= 0x036F) ||
         (value >= 0x1AB0 && value <= 0x1AFF) ||
         (value >= 0x1DC0 && value <= 0x1DFF) ||
         (value >= 0x20D0 && value <= 0x20FF) ||
         (value >= 0xFE20 && value <= 0xFE2F) ||
         value == 0x200D || (value >= 0xFE00 && value <= 0xFE0F);
}

static size_t __unicode_width(unsigned long value) {
  if (value == 0 || value < 0x20 || (value >= 0x7F && value < 0xA0) ||
      __unicode_combining(value)) {
    return 0;
  }
  if ((value >= 0x1100 && value <= 0x115F) ||
      (value >= 0x2329 && value <= 0x232A) ||
      (value >= 0x2E80 && value <= 0xA4CF) ||
      (value >= 0xAC00 && value <= 0xD7A3) ||
      (value >= 0xF900 && value <= 0xFAFF) ||
      (value >= 0xFE10 && value <= 0xFE19) ||
      (value >= 0xFE30 && value <= 0xFE6F) ||
      (value >= 0xFF00 && value <= 0xFF60) ||
      (value >= 0xFFE0 && value <= 0xFFE6) ||
      (value >= 0x1F300 && value <= 0x1FAFF) ||
      (value >= 0x20000 && value <= 0x3FFFD)) {
    return 2;
  }
  return 1;
}

size_t __utf8_display_width(string_view value) {
  size_t width = 0;
  for (size_t position = 0; position < value.size();) {
    unsigned long code_point = 0;
    size_t length = 1;
    __utf8_decode(value, position, &code_point, &length);
    width += __unicode_width(code_point);
    position += length;
  }
  return width;
}

size_t __utf8_prefix_for_width(string_view value, size_t maximum_width) {
  size_t position = 0;
  size_t accepted = 0;
  size_t width = 0;
  while (position < value.size()) {
    unsigned long code_point = 0;
    size_t length = 1;
    __utf8_decode(value, position, &code_point, &length);
    size_t code_width = __unicode_width(code_point);
    if (code_width != 0 && width + code_width > maximum_width) {
      break;
    }
    width += code_width;
    position += length;
    accepted = position;
  }
  return accepted;
}

static void __append_hex_escape(unsigned char value, string* output) {
  static const char digits[] = "0123456789abcdef";
  output->append("\\x", 2);
  output->push_back(digits[value >> 4]);
  output->push_back(digits[value & 15]);
}

void __append_debug_string(string_view value, bool character, string* output) {
  const char quote = character ? '\'' : '"';
  output->push_back(quote);
  for (size_t position = 0; position < value.size();) {
    unsigned char byte = static_cast<unsigned char>(value[position]);
    if (byte == static_cast<unsigned char>(quote) || byte == '\\') {
      output->push_back('\\');
      output->push_back(static_cast<char>(byte));
      ++position;
    } else if (byte == '\n' || byte == '\r' || byte == '\t') {
      output->push_back('\\');
      output->push_back(byte == '\n' ? 'n' : byte == '\r' ? 'r' : 't');
      ++position;
    } else if (byte < 0x20 || byte == 0x7F) {
      __append_hex_escape(byte, output);
      ++position;
    } else if (byte < 0x80) {
      output->push_back(static_cast<char>(byte));
      ++position;
    } else {
      unsigned long code_point = 0;
      size_t length = 1;
      bool valid = __utf8_decode(value, position, &code_point, &length);
      (void)code_point;
      if (!valid) {
        __append_hex_escape(byte, output);
        ++position;
      } else {
        output->append(value.data() + position, length);
        position += length;
      }
    }
  }
  output->push_back(quote);
}

void __append_padded(string* output, const string* value,
                     const __spec* spec, bool numeric, bool unicode_text) {
  size_t width = spec->width > 0 ? static_cast<size_t>(spec->width) : 0;
  size_t value_width =
      unicode_text
          ? __utf8_display_width(string_view(value->data(), value->size()))
          : value->size();
  if (width <= value_width) {
    output->append(*value);
    return;
  }
  size_t padding = width - value_width;
  char align = spec->align;
  if (align == 0) {
    align = numeric ? '>' : '<';
  }
  if (spec->zero && numeric && spec->align == 0) {
    size_t prefix = __prefix_length(*value);
    output->append(value->data(), prefix);
    __append_repeat(*output, '0', padding);
    output->append(value->data() + prefix, value->size() - prefix);
  } else if (align == '<') {
    output->append(*value);
    __append_repeat(*output, spec->fill, padding);
  } else if (align == '^') {
    size_t left = padding / 2;
    __append_repeat(*output, spec->fill, left);
    output->append(*value);
    __append_repeat(*output, spec->fill, padding - left);
  } else {
    __append_repeat(*output, spec->fill, padding);
    output->append(*value);
  }
}

static string __binary_impl(unsigned long long value, bool upper,
                            bool alternate) {
  char buffer[sizeof(unsigned long long) * 8 + 2];
  size_t position = sizeof(buffer);
  do {
    buffer[--position] = '0' + (value & 1);
    value >>= 1;
  } while (value != 0);
  if (alternate) {
    buffer[--position] = upper ? 'B' : 'b';
    buffer[--position] = '0';
  }
  return string(buffer + position, sizeof(buffer) - position);
}

static string __unsigned_integer_impl(unsigned long long value, char type,
                                      bool alternate) {
  if (type == 'b' || type == 'B') {
    return __binary_impl(value, type == 'B', alternate);
  }
  unsigned char base = type == 'x' || type == 'X' ? 16
                       : type == 'o'               ? 8
                                                   : 10;
  unsigned char flags = 0;
  if (type == 'X') {
    flags |= __DAVECC_ITOA_UPPER;
  }
  if (alternate) {
    flags |= __DAVECC_ITOA_SHOWBASE;
  }
  char buffer[__DAVECC_ITOA_CAPACITY(unsigned long long)];
  size_t length = __utoa_ulonglong(buffer, value, base, flags);
  return string(buffer, length);
}

static string __signed_integer_impl(long long value, const __spec& spec) {
  char type = spec.type == 0 ? 'd' : spec.type;
  if (type != 'd' && type != 'x' && type != 'X' && type != 'o' &&
      type != 'b' && type != 'B' && type != 'c') {
    __fail("invalid integer presentation type");
  }
  if (type == 'c') {
    char character = (char)(static_cast<unsigned long long>(value) & 0xFF);
    return string(1, character);
  }
  if (type == 'd') {
    unsigned char flags = spec.sign == '+' ? __DAVECC_ITOA_SHOWPOS : 0;
    char buffer[__DAVECC_ITOA_CAPACITY(long long)];
    size_t length = __itoa_longlong(buffer, value, 10, flags);
    string result(buffer, length);
    if (spec.sign == ' ' && value >= 0) {
      string prefixed(1, ' ');
      prefixed.append(result);
      result = static_cast<string&&>(prefixed);
    }
    return result;
  }
  return __unsigned_integer_impl(static_cast<unsigned long long>(value), type,
                                 spec.alternate);
}

static string __unsigned_integer_with_sign_impl(unsigned long long value,
                                                const __spec& spec) {
  char type = spec.type == 0 ? 'd' : spec.type;
  if (type == 'c') {
    char character = (char)(value & 0xFF);
    return string(1, character);
  }
  if (type != 'd' && type != 'x' && type != 'X' && type != 'o' &&
      type != 'b' && type != 'B') {
    __fail("invalid integer presentation type");
  }
  string result = __unsigned_integer_impl(value, type, spec.alternate);
  if (type == 'd' && (spec.sign == '+' || spec.sign == ' ')) {
    string prefixed(1, spec.sign);
    prefixed.append(result);
    result = static_cast<string&&>(prefixed);
  }
  return result;
}

static string __floating_impl(double value, const __spec& spec) {
  char type = spec.type == 0 ? 'g' : spec.type;
  bool upper = type >= 'A' && type <= 'Z';
  char lower = upper ? static_cast<char>(type + ('a' - 'A')) : type;
  if (lower != 'f' && lower != 'e' && lower != 'g') {
    __fail("invalid floating-point presentation type");
  }
  int precision = spec.precision >= 0 ? spec.precision : 6;
  char buffer[96];
  char* converted = lower == 'f'
                        ? __PrintFloatFormat(value, precision, buffer,
                                             sizeof(buffer))
                        : lower == 'e'
                              ? __PrintScientificFormat(value, precision, buffer,
                                                        sizeof(buffer))
                              : __PrintGeneralFormat(value, precision, buffer,
                                                     sizeof(buffer));
  string result(converted);
  if (spec.sign == '+' && (result.empty() || result[0] != '-')) {
    string prefixed(1, '+');
    prefixed.append(result);
    result = static_cast<string&&>(prefixed);
  } else if (spec.sign == ' ' && (result.empty() || result[0] != '-')) {
    string prefixed(1, ' ');
    prefixed.append(result);
    result = static_cast<string&&>(prefixed);
  }
  bool point = false;
  for (size_t i = 0; i < result.size(); ++i) {
    if (result[i] == '.') {
      point = true;
    }
    if (upper && result[i] >= 'a' && result[i] <= 'z') {
      result[i] = static_cast<char>(result[i] - ('a' - 'A'));
    }
  }
  if (spec.alternate && !point) {
    result.push_back('.');
  }
  return result;
}

void __unsigned_integer_to(unsigned long long value, char type, bool alternate,
                           string* output) {
  *output = __unsigned_integer_impl(value, type, alternate);
}

void __signed_integer_to(long long value, const __spec& spec, string* output) {
  char type = spec.type == 0 ? 'd' : spec.type;
  if (type == 'c') {
    output->clear();
    output->push_back((char)(static_cast<unsigned long long>(value) & 0xFF));
    return;
  }
  if (type == 'd') {
    unsigned char flags = spec.sign == '+' ? __DAVECC_ITOA_SHOWPOS : 0;
    char buffer[__DAVECC_ITOA_CAPACITY(long long)];
    size_t length = __itoa_longlong(buffer, value, 10, flags);
    output->clear();
    if (spec.sign == ' ' && value >= 0) {
      output->push_back(' ');
    }
    output->append(buffer, length);
    return;
  }
  *output = __signed_integer_impl(value, spec);
}

void __unsigned_integer_with_sign_to(unsigned long long value,
                                     const __spec& spec, string* output) {
  *output = __unsigned_integer_with_sign_impl(value, spec);
}

void __floating_to(double value, const __spec& spec, string* output) {
  *output = __floating_impl(value, spec);
}

string __unsigned_integer(unsigned long long value, char type, bool alternate) {
  string output;
  __unsigned_integer_to(value, type, alternate, &output);
  return output;
}

string __signed_integer(long long value, const __spec& spec) {
  string output;
  __signed_integer_to(value, spec, &output);
  return output;
}

string __unsigned_integer_with_sign(unsigned long long value,
                                    const __spec& spec) {
  string output;
  __unsigned_integer_with_sign_to(value, spec, &output);
  return output;
}

string __floating(double value, const __spec& spec) {
  string output;
  __floating_to(value, spec, &output);
  return output;
}

void __vformat_to(const char* text_data, size_t text_size,
                  const __format_args* arguments, string* output_pointer) {
  if (text_data == nullptr) {
    text_data = "";
    text_size = 0;
  }
  if (arguments == nullptr || output_pointer == nullptr) {
    __fail("invalid format invocation");
  }
  __format_args args = *arguments;
  size_t position = 0;
  size_t next_argument = 0;
  bool automatic = false;
  bool manual = false;
  while (position < text_size) {
    char current = text_data[position++];
    if (current == '{') {
      if (position < text_size && text_data[position] == '{') {
        output_pointer->push_back('{');
        ++position;
        continue;
      }
      size_t argument = 0;
      bool explicit_argument = false;
      while (position < text_size && __is_digit(text_data[position])) {
        explicit_argument = true;
        argument =
            argument * 10 + static_cast<size_t>(text_data[position] - '0');
        ++position;
      }
      if (explicit_argument) {
        manual = true;
        if (automatic) {
          __fail("cannot mix automatic and manual argument indices");
        }
      } else {
        automatic = true;
        if (manual) {
          __fail("cannot mix automatic and manual argument indices");
        }
        argument = next_argument++;
      }
      const char* specification_data = nullptr;
      size_t specification_start = 0;
      size_t specification_size = 0;
      if (position < text_size && text_data[position] == ':') {
        specification_start = ++position;
        int nested = 0;
        while (position < text_size) {
          if (text_data[position] == '{') {
            ++nested;
          } else if (text_data[position] == '}') {
            if (nested == 0) {
              break;
            }
            --nested;
          }
          ++position;
        }
        specification_data = text_data + specification_start;
        specification_size = position - specification_start;
      }
      if (position >= text_size || text_data[position] != '}') {
        __fail("unmatched '{' in format string");
      }
      ++position;
      const __format_arg* value = args.get(argument);
      if (value == nullptr || value->format_value == nullptr) {
        __fail("format argument index out of range");
      }
      args.next_dynamic = next_argument;
      __spec parsed_specification;
      size_t spec_position = specification_start;
      size_t spec_end =
          specification_start + (value->is_builtin ? specification_size : 0);
      if (spec_position + 1 < spec_end &&
          (text_data[spec_position + 1] == '<' ||
           text_data[spec_position + 1] == '>' ||
           text_data[spec_position + 1] == '^')) {
        parsed_specification.fill = text_data[spec_position];
        parsed_specification.align = text_data[spec_position + 1];
        spec_position += 2;
      } else if (spec_position < spec_end &&
                 (text_data[spec_position] == '<' ||
                  text_data[spec_position] == '>' ||
                  text_data[spec_position] == '^')) {
        parsed_specification.align = text_data[spec_position++];
      }
      if (spec_position < spec_end &&
          (text_data[spec_position] == '+' || text_data[spec_position] == '-' ||
           text_data[spec_position] == ' ')) {
        parsed_specification.sign = text_data[spec_position++];
      }
      if (spec_position < spec_end && text_data[spec_position] == '#') {
        parsed_specification.alternate = true;
        ++spec_position;
      }
      if (spec_position < spec_end && text_data[spec_position] == '0') {
        parsed_specification.zero = true;
        ++spec_position;
      }
      if (spec_position < spec_end && __is_digit(text_data[spec_position])) {
        parsed_specification.width = 0;
        while (spec_position < spec_end &&
               __is_digit(text_data[spec_position])) {
          parsed_specification.width =
              parsed_specification.width * 10 +
              (text_data[spec_position] - '0');
          ++spec_position;
        }
      } else if (spec_position < spec_end &&
                 text_data[spec_position] == '{') {
        ++spec_position;
        size_t id = 0;
        bool explicit_id = false;
        while (spec_position < spec_end &&
               __is_digit(text_data[spec_position])) {
          explicit_id = true;
          id = id * 10 +
               static_cast<size_t>(text_data[spec_position] - '0');
          ++spec_position;
        }
        if (spec_position >= spec_end || text_data[spec_position] != '}') {
          __fail("invalid dynamic format specification");
        }
        ++spec_position;
        if (!explicit_id) {
          id = args.next_dynamic++;
        }
        parsed_specification.width = __dynamic_integer(&args, id);
      }
      if (spec_position < spec_end && text_data[spec_position] == '.') {
        ++spec_position;
        if (spec_position < spec_end &&
            __is_digit(text_data[spec_position])) {
          parsed_specification.precision = 0;
          while (spec_position < spec_end &&
                 __is_digit(text_data[spec_position])) {
            parsed_specification.precision =
                parsed_specification.precision * 10 +
                (text_data[spec_position] - '0');
            ++spec_position;
          }
        } else if (spec_position < spec_end &&
                   text_data[spec_position] == '{') {
          ++spec_position;
          size_t id = 0;
          bool explicit_id = false;
          while (spec_position < spec_end &&
                 __is_digit(text_data[spec_position])) {
            explicit_id = true;
            id = id * 10 +
                 static_cast<size_t>(text_data[spec_position] - '0');
            ++spec_position;
          }
          if (spec_position >= spec_end || text_data[spec_position] != '}') {
            __fail("invalid dynamic format specification");
          }
          ++spec_position;
          if (!explicit_id) {
            id = args.next_dynamic++;
          }
          parsed_specification.precision =
              __dynamic_integer(&args, id);
        } else {
          __fail("missing precision");
        }
      }
      if (spec_position < spec_end && text_data[spec_position] == 'L') {
        parsed_specification.localized = true;
        ++spec_position;
      }
      if (spec_position < spec_end) {
        parsed_specification.type = text_data[spec_position++];
      }
      if (spec_position != spec_end) {
        __fail("invalid format specification");
      }
      __format_specification specification = {
          &parsed_specification, specification_data, specification_size};
      string formatted;
      value->format_value(value->value, &specification, formatted, &args);
      output_pointer->append(formatted);
      next_argument = args.next_dynamic;
    } else if (current == '}') {
      if (position < text_size && text_data[position] == '}') {
        output_pointer->push_back('}');
        ++position;
      } else {
        __fail("unmatched '}' in format string");
      }
    } else {
      output_pointer->push_back(current);
    }
  }
}

static void AppendDecimal(string* output, long long value, int width) {
  bool negative = value < 0;
  unsigned long long magnitude =
      negative ? static_cast<unsigned long long>(-(value + 1)) + 1
               : static_cast<unsigned long long>(value);
  string digits;
  __unsigned_integer_to(magnitude, 'd', false, &digits);
  if (negative) {
    output->push_back('-');
  }
  for (int i = static_cast<int>(digits.size()); i < width; ++i) {
    output->push_back('0');
  }
  output->append(digits);
}

static void AppendDurationUnit(string* output, intmax_t num, intmax_t den) {
  if (num == 1 && den == 1000000000) {
    output->append("ns");
  } else if (num == 1 && den == 1000000) {
    output->append("us");
  } else if (num == 1 && den == 1000) {
    output->append("ms");
  } else if (num == 1 && den == 1) {
    output->push_back('s');
  } else if (num == 60 && den == 1) {
    output->append("min");
  } else if (num == 3600 && den == 1) {
    output->push_back('h');
  } else if (num == 86400 && den == 1) {
    output->push_back('d');
  } else {
    output->push_back('[');
    AppendDecimal(output, num, 0);
    if (den != 1) {
      output->push_back('/');
      AppendDecimal(output, den, 0);
    }
    output->append("]s");
  }
}

void __format_chrono_duration(string_view count, long double total_seconds,
                              intmax_t period_num, intmax_t period_den,
                              const char* specification_data,
                              size_t specification_size, string* output) {
  string_view specification(specification_data, specification_size);
  if (specification.empty()) {
    output->append(count.data(), count.size());
    AppendDurationUnit(output, period_num, period_den);
    return;
  }

  long long whole_seconds = static_cast<long long>(total_seconds);
  long long magnitude = whole_seconds < 0 ? -whole_seconds : whole_seconds;
  for (size_t i = 0; i < specification.size(); ++i) {
    char current = specification[i];
    if (current != '%') {
      output->push_back(current);
      continue;
    }
    if (++i >= specification.size()) {
      __fail("incomplete chrono format specifier");
    }
    switch (specification[i]) {
      case '%':
        output->push_back('%');
        break;
      case 'n':
        output->push_back('\n');
        break;
      case 't':
        output->push_back('\t');
        break;
      case 'Q':
        output->append(count.data(), count.size());
        break;
      case 'q':
        AppendDurationUnit(output, period_num, period_den);
        break;
      case 'H':
        AppendDecimal(output, (magnitude / 3600) % 24, 2);
        break;
      case 'M':
        AppendDecimal(output, (magnitude / 60) % 60, 2);
        break;
      case 'S':
        AppendDecimal(output, magnitude % 60, 2);
        break;
      case 'R':
        AppendDecimal(output, (magnitude / 3600) % 24, 2);
        output->push_back(':');
        AppendDecimal(output, (magnitude / 60) % 60, 2);
        break;
      case 'T':
        AppendDecimal(output, (magnitude / 3600) % 24, 2);
        output->push_back(':');
        AppendDecimal(output, (magnitude / 60) % 60, 2);
        output->push_back(':');
        AppendDecimal(output, magnitude % 60, 2);
        break;
      case 'j':
        AppendDecimal(output, magnitude / 86400, 0);
        break;
      default:
        __fail("unsupported chrono duration format specifier");
    }
  }
}

static long long FloorDiv(long long value, long long divisor) {
  long long quotient = value / divisor;
  long long remainder = value % divisor;
  return remainder < 0 ? quotient - 1 : quotient;
}

struct CivilTime {
  long long year;
  unsigned month;
  unsigned day;
  unsigned hour;
  unsigned minute;
  unsigned second;
};

static CivilTime CivilTimeFromEpoch(long long seconds_since_epoch) {
  long long days = FloorDiv(seconds_since_epoch, 86400);
  long long day_seconds = seconds_since_epoch - days * 86400;
  long long z = days + 719468;
  long long era = FloorDiv(z, 146097);
  unsigned day_of_era = static_cast<unsigned>(z - era * 146097);
  unsigned year_of_era =
      (day_of_era - day_of_era / 1460 + day_of_era / 36524 -
       day_of_era / 146096) /
      365;
  long long year = static_cast<long long>(year_of_era) + era * 400;
  unsigned day_of_year =
      day_of_era -
      (365 * year_of_era + year_of_era / 4 - year_of_era / 100);
  unsigned month_prime = (5 * day_of_year + 2) / 153;
  unsigned day =
      day_of_year - (153 * month_prime + 2) / 5 + 1;
  unsigned month = month_prime < 10 ? month_prime + 3 : month_prime - 9;
  year += month <= 2;
  CivilTime result;
  result.year = year;
  result.month = month;
  result.day = day;
  result.hour = static_cast<unsigned>(day_seconds / 3600);
  result.minute = static_cast<unsigned>((day_seconds / 60) % 60);
  result.second = static_cast<unsigned>(day_seconds % 60);
  return result;
}

void __format_chrono_time_point(long long seconds_since_epoch,
                                const char* specification_data,
                                size_t specification_size, string* output) {
  string_view specification(specification_data, specification_size);
  CivilTime value = CivilTimeFromEpoch(seconds_since_epoch);
  const char* active_data =
      specification.empty() ? "%F %T" : specification.data();
  size_t active_size = specification.empty() ? 5 : specification.size();
  for (size_t i = 0; i < active_size; ++i) {
    char current = active_data[i];
    if (current != '%') {
      output->push_back(current);
      continue;
    }
    if (++i >= active_size) {
      __fail("incomplete chrono format specifier");
    }
    switch (active_data[i]) {
      case '%':
        output->push_back('%');
        break;
      case 'n':
        output->push_back('\n');
        break;
      case 't':
        output->push_back('\t');
        break;
      case 'Y':
        AppendDecimal(output, value.year, 4);
        break;
      case 'm':
        AppendDecimal(output, value.month, 2);
        break;
      case 'd':
        AppendDecimal(output, value.day, 2);
        break;
      case 'H':
        AppendDecimal(output, value.hour, 2);
        break;
      case 'M':
        AppendDecimal(output, value.minute, 2);
        break;
      case 'S':
        AppendDecimal(output, value.second, 2);
        break;
      case 'F':
        AppendDecimal(output, value.year, 4);
        output->push_back('-');
        AppendDecimal(output, value.month, 2);
        output->push_back('-');
        AppendDecimal(output, value.day, 2);
        break;
      case 'R':
        AppendDecimal(output, value.hour, 2);
        output->push_back(':');
        AppendDecimal(output, value.minute, 2);
        break;
      case 'T':
        AppendDecimal(output, value.hour, 2);
        output->push_back(':');
        AppendDecimal(output, value.minute, 2);
        output->push_back(':');
        AppendDecimal(output, value.second, 2);
        break;
      default:
        __fail("unsupported chrono time-point format specifier");
    }
  }
}

string __format_invoke(const char* text_data, size_t text_size) {
  string output;
  __format_args runtime_args(nullptr, 0);
  __vformat_to(text_data, text_size, &runtime_args, &output);
  return output;
}

}  // namespace __format_detail

string vformat(string_view text, format_args args) {
  string output;
  __format_detail::__format_args runtime_args = args.__implementation();
  __format_detail::__vformat_to(text.data(), text.size(), &runtime_args,
                                &output);
  return output;
}

string format(
    const typename __format_detail::__non_deduced<format_string<>>::type& text) {
  return __format_detail::__format_invoke(text.data(), text.size());
}

}  // namespace std
