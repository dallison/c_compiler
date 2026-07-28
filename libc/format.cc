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

void __append_padded(string* output, const string* value,
                     const __spec* spec, bool numeric) {
  size_t width = spec->width > 0 ? static_cast<size_t>(spec->width) : 0;
  if (width <= value->size()) {
    output->append(*value);
    return;
  }
  size_t padding = width - value->size();
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
      size_t spec_end = specification_start + specification_size;
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

__format_args __make_runtime_args(const __format_arg_store& store) {
  __format_args args;
  args.values = store.__data();
  args.count = store.__size();
  args.next_dynamic = 0;
  return args;
}

string __format_invoke(const char* text_data, size_t text_size) {
  __format_arg_store store;
  string output;
  __format_args runtime_args = __make_runtime_args(store);
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

__format_detail::__format_arg_store make_format_args() {
  return __format_detail::__format_arg_store();
}

}  // namespace std
