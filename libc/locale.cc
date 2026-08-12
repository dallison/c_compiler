#include <locale>

#include <__itoa.h>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <istream>
#include <streambuf>
#include <typeinfo>

// locale runtime: classic facet registry and iostream locale hooks.

extern "C" char* __PrintFloatFormat(double, int, char*, size_t);
extern "C" char* __PrintScientificFormat(double, int, char*, size_t);
extern "C" char* __PrintGeneralFormat(double, int, char*, size_t);
#ifndef __cpp_exceptions
extern "C" void __davecc_raise_bad_cast(void);
#endif

namespace std {

struct __locale_impl {
  static const size_t __max_facets_ = 16;
  mutable size_t __refs;
  const char* __name;
  size_t __facet_count;
  locale::facet* __facets[__max_facets_];

  explicit __locale_impl(const char* name);
  __locale_impl(const __locale_impl& other);
  ~__locale_impl();

  locale::facet* __get(size_t index) const {
    return index < __facet_count ? __facets[index] : nullptr;
  }
  bool __has(size_t index) const { return __get(index) != nullptr; }
  void __install(locale::facet* facet, size_t index);
  __locale_impl* __combine_with(const __locale_impl* source,
                                locale::category cat) const;
};

ios_base::event_callback ios_base::__callbacks_[ios_base::__max_callbacks_];
int ios_base::__callback_indices_[ios_base::__max_callbacks_];
int ios_base::__callback_count_ = 0;

namespace __locale_detail {

size_t __next_facet_index() {
  static size_t next = 0;
  return next++;
}

[[noreturn]] void __throw_runtime(const char* message) {
  __DAVECC_THROW(runtime_error(message));
}

[[noreturn]] void __throw_bad_cast() {
#ifdef __cpp_exceptions
  throw bad_cast();
#else
  __davecc_raise_bad_cast();
#endif
}

}  // namespace __locale_detail

enum {
  __cat_collate = 1,
  __cat_ctype = 2,
  __cat_monetary = 4,
  __cat_numeric = 8,
  __cat_time = 16,
  __cat_messages = 32,
};

struct __installed_facet {
  locale::facet* facet;
  int category;
};

template <class Facet>
inline void __install_facet(__locale_impl* impl, locale::facet* facet) {
  impl->__install(facet, Facet::id.__index());
}

template <class Facet>
inline Facet* __make_facet() {
  return new Facet;
}

inline ctype_base::mask __mask_from_c(int c, unsigned char uc) {
  ctype_base::mask m = 0;
  if (isspace(uc)) {
    m |= ctype_base::space;
  }
  if (isprint(uc)) {
    m |= ctype_base::print;
  }
  if (iscntrl(uc)) {
    m |= ctype_base::cntrl;
  }
  if (isupper(uc)) {
    m |= ctype_base::upper;
  }
  if (islower(uc)) {
    m |= ctype_base::lower;
  }
  if (isalpha(uc)) {
    m |= ctype_base::alpha;
  }
  if (isdigit(uc)) {
    m |= ctype_base::digit;
  }
  if (ispunct(uc)) {
    m |= ctype_base::punct;
  }
  if (isxdigit(uc)) {
    m |= ctype_base::xdigit;
  }
  if (uc == ' ' || uc == '\t') {
    m |= ctype_base::blank;
  }
  if ((m & ctype_base::alpha) || (m & ctype_base::digit) ||
      (m & ctype_base::punct)) {
    m |= ctype_base::graph;
  }
  (void)c;
  return m;
}

unsigned short __debug_classic_mask(unsigned char c) {
  return __mask_from_c(c, c);
}

ostreambuf_iterator __write_chars(ostreambuf_iterator out, ios_base& str,
                                  char fill, const char* buf, size_t len) {
  streamsize width = str.width();
  if (width > 0 && static_cast<streamsize>(len) < width) {
    streamsize pad_count = width - static_cast<streamsize>(len);
    ios_base::fmtflags flags = str.flags();
    if ((flags & ios_base::left) != 0) {
      for (size_t i = 0; i < len; ++i) {
        out = buf[i];
      }
      for (streamsize i = 0; i < pad_count; ++i) {
        out = fill;
      }
    } else {
      for (streamsize i = 0; i < pad_count; ++i) {
        out = fill;
      }
      for (size_t i = 0; i < len; ++i) {
        out = buf[i];
      }
    }
    str.width(0);
  } else {
    for (size_t i = 0; i < len; ++i) {
      out = buf[i];
    }
  }
  return out;
}

wostreambuf_iterator __write_wchars(wostreambuf_iterator out, ios_base& str,
                                    wchar_t fill, const wchar_t* buf,
                                    size_t len) {
  streamsize width = str.width();
  if (width > 0 && static_cast<streamsize>(len) < width) {
    streamsize pad_count = width - static_cast<streamsize>(len);
    ios_base::fmtflags flags = str.flags();
    if ((flags & ios_base::left) != 0) {
      for (size_t i = 0; i < len; ++i) {
        out = buf[i];
      }
      for (streamsize i = 0; i < pad_count; ++i) {
        out = fill;
      }
    } else {
      for (streamsize i = 0; i < pad_count; ++i) {
        out = fill;
      }
      for (size_t i = 0; i < len; ++i) {
        out = buf[i];
      }
    }
    str.width(0);
  } else {
    for (size_t i = 0; i < len; ++i) {
      out = buf[i];
    }
  }
  return out;
}

ostreambuf_iterator __write_c_string(ostreambuf_iterator out, ios_base& str,
                                     char fill, const char* buf, size_t len) {
  return __write_chars(out, str, fill, buf, len);
}

wostreambuf_iterator __write_wc_string(wostreambuf_iterator out, ios_base& str,
                                       wchar_t fill, const char* buf,
                                       size_t len) {
  streamsize width = str.width();
  if (width > 0 && static_cast<streamsize>(len) < width) {
    streamsize pad_count = width - static_cast<streamsize>(len);
    ios_base::fmtflags flags = str.flags();
    if ((flags & ios_base::left) != 0) {
      for (size_t i = 0; i < len; ++i) {
        out = static_cast<wchar_t>(buf[i]);
      }
      for (streamsize i = 0; i < pad_count; ++i) {
        out = fill;
      }
    } else {
      for (streamsize i = 0; i < pad_count; ++i) {
        out = fill;
      }
      for (size_t i = 0; i < len; ++i) {
        out = static_cast<wchar_t>(buf[i]);
      }
    }
    str.width(0);
  } else {
    for (size_t i = 0; i < len; ++i) {
      out = static_cast<wchar_t>(buf[i]);
    }
  }
  return out;
}

template <class CharT, class Iter>
bool __read_digit(CharT c, int base, int& value) {
  int digit = -1;
  if (c >= CharT('0') && c <= CharT('9')) {
    digit = static_cast<int>(c - CharT('0'));
  } else if (base > 10 && c >= CharT('a') && c <= CharT('f')) {
    digit = 10 + static_cast<int>(c - CharT('a'));
  } else if (base > 10 && c >= CharT('A') && c <= CharT('F')) {
    digit = 10 + static_cast<int>(c - CharT('A'));
  }
  if (digit < 0 || digit >= base) {
    return false;
  }
  value = value * base + digit;
  return true;
}

template <class CharT, class Iter, class T>
Iter __get_unsigned_integer(Iter in, Iter end, ios_base& str,
                            ios_base::iostate& err, T& value) {
  using traits = char_traits<CharT>;
  while (in != end && traits::eq(*in, CharT(' '))) {
    ++in;
  }
  ios_base::fmtflags flags = str.flags();
  int base = 10;
  if ((flags & ios_base::basefield) == ios_base::hex) {
    base = 16;
  } else if ((flags & ios_base::basefield) == ios_base::oct) {
    base = 8;
  }
  Iter start = in;
  T result = 0;
  bool any = false;
  if (in != end && traits::eq(*in, CharT('0')) &&
      (base == 8 || base == 16)) {
    ++in;
    any = true;
    if (base == 16 && in != end &&
        (traits::eq(*in, CharT('x')) || traits::eq(*in, CharT('X')))) {
      ++in;
      start = in;
      any = false;
    }
  }
  while (in != end) {
    int digit_value = 0;
    if (!__read_digit<CharT, Iter>(static_cast<CharT>(*in), base, digit_value)) {
      break;
    }
    result = static_cast<T>(result * base + static_cast<T>(digit_value));
    any = true;
    ++in;
  }
  if (!any) {
    err = ios_base::failbit;
    return start;
  }
  value = result;
  return in;
}

__locale_impl::__locale_impl(const char* name)
    : __refs(0), __name(name != nullptr ? name : "C"), __facet_count(0) {
  for (size_t i = 0; i < __max_facets_; ++i) {
    __facets[i] = nullptr;
  }
}

__locale_impl::__locale_impl(const __locale_impl& other)
    : __refs(0), __name(other.__name), __facet_count(other.__facet_count) {
  for (size_t i = 0; i < __max_facets_; ++i) {
    __facets[i] = other.__facets[i];
    if (__facets[i] != nullptr) {
      __facets[i]->__add_ref();
    }
  }
}

__locale_impl::~__locale_impl() {
  for (size_t i = 0; i < __facet_count; ++i) {
    if (__facets[i] != nullptr) {
      __facets[i]->__release();
    }
  }
}

void __locale_impl::__install(locale::facet* facet, size_t index) {
  if (facet == nullptr || index >= __max_facets_) {
    return;
  }
  if (index >= __facet_count) {
    __facet_count = index + 1;
  }
  if (__facets[index] != nullptr) {
    __facets[index]->__release();
  }
  __facets[index] = facet;
  facet->__add_ref();
}

__locale_impl* __locale_impl::__combine_with(const __locale_impl* source,
                                             locale::category cat) const {
  __locale_impl* result = new __locale_impl(*this);
  if (source == nullptr) {
    return result;
  }
  result->__name = "*";

  if ((cat & locale::ctype) != 0) {
    locale::facet* facet = source->__get(ctype<char>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, ctype<char>::id.__index());
    }
    facet = source->__get(ctype<wchar_t>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, ctype<wchar_t>::id.__index());
    }
  }
  if ((cat & locale::numeric) != 0) {
    locale::facet* facet = source->__get(numpunct<char>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, numpunct<char>::id.__index());
    }
    facet = source->__get(numpunct<wchar_t>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, numpunct<wchar_t>::id.__index());
    }
    facet = source->__get(num_put<char, ostreambuf_iterator>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, num_put<char, ostreambuf_iterator>::id.__index());
    }
    facet = source->__get(num_put<wchar_t, wostreambuf_iterator>::id.__index());
    if (facet != nullptr) {
      result->__install(facet,
                        num_put<wchar_t, wostreambuf_iterator>::id.__index());
    }
    facet = source->__get(num_get<char, istreambuf_iterator>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, num_get<char, istreambuf_iterator>::id.__index());
    }
    facet = source->__get(num_get<wchar_t, wistreambuf_iterator>::id.__index());
    if (facet != nullptr) {
      result->__install(facet,
                        num_get<wchar_t, wistreambuf_iterator>::id.__index());
    }
  }
  return result;
}

locale::id ctype<char>::id;
locale::id ctype<wchar_t>::id;
locale::id numpunct<char>::id;
locale::id numpunct<wchar_t>::id;
locale::id __davecc_num_put_char::id;
locale::id __davecc_num_put_wchar::id;
locale::id __davecc_num_get_char::id;
locale::id __davecc_num_get_wchar::id;

const locale::category locale::none = 0;
const locale::category locale::collate = 1;
const locale::category locale::ctype = 2;
const locale::category locale::monetary = 4;
const locale::category locale::numeric = 8;
const locale::category locale::time = 16;
const locale::category locale::messages = 32;
const locale::category locale::all =
    locale::collate | locale::ctype | locale::monetary | locale::numeric |
    locale::time | locale::messages;

locale::facet* locale::__get_facet(locale::id& facet_id) const {
  return __impl_ != nullptr ? __impl_->__get(facet_id.__index()) : nullptr;
}

const locale::facet* locale::__get_facet(const locale::id& facet_id) const {
  return __impl_ != nullptr ? __impl_->__get(facet_id.__index()) : nullptr;
}

bool locale::__has_facet(const locale::id& facet_id) const {
  return __impl_ != nullptr ? __impl_->__has(facet_id.__index()) : false;
}

ctype_base::mask ctype<char>::__classic_mask(unsigned char c) {
  return __mask_from_c(c, c);
}

ctype_base::mask ctype<wchar_t>::__classic_wmask(wchar_t c) {
  if (c >= 0 && c <= 0x7f) {
    return __mask_from_c(static_cast<int>(c), static_cast<unsigned char>(c));
  }
  return ctype_base::cntrl;
}

static size_t __locale_format_ll(char* buf, long long v, int base) {
  unsigned char flags = 0;
  if (base == 16) {
    flags |= __DAVECC_ITOA_UPPER;
  }
  return __itoa_longlong(buf, v, static_cast<unsigned char>(base), flags);
}

static size_t __locale_format_ull(char* buf, unsigned long long v, int base) {
  unsigned char flags = 0;
  if (base == 16) {
    flags |= __DAVECC_ITOA_UPPER;
  }
  return __utoa_ulonglong(buf, v, static_cast<unsigned char>(base), flags);
}

static size_t __locale_format_ptr(char* buf, const void* v) {
  return __ptrtoa(buf, v);
}


ostreambuf_iterator __davecc_classic_put_bool(ostreambuf_iterator s, ios_base& f, char fill, bool v) {
  const numpunct<char>& np = use_facet<numpunct<char>>(f.getloc());
  if ((f.flags() & ios_base::boolalpha) != 0) {
    basic_string<char> name = v ? np.truename() : np.falsename();
    return __write_chars(s, f, fill, name.data(), name.size());
  }
  return __davecc_classic_put_long(s, f, fill, static_cast<long>(v ? 1 : 0));
}

ostreambuf_iterator __davecc_classic_put_long(ostreambuf_iterator s, ios_base& f, char fill, long v) {
  return __davecc_classic_put_llong(s, f, fill, static_cast<long long>(v));
}

ostreambuf_iterator __davecc_classic_put_ulong(ostreambuf_iterator s, ios_base& f, char fill, unsigned long v) {
  return __davecc_classic_put_ullong(s, f, fill, static_cast<unsigned long long>(v));
}

ostreambuf_iterator __davecc_classic_put_llong(ostreambuf_iterator s, ios_base& f, char fill, long long v) {
  char buf[__DAVECC_ITOA_CAPACITY(long long)];
  ios_base::fmtflags flags = f.flags();
  int base = 10;
  if ((flags & ios_base::basefield) == ios_base::hex) {
    base = 16;
  } else if ((flags & ios_base::basefield) == ios_base::oct) {
    base = 8;
  }
  size_t len = __locale_format_ll(buf, v, base);
  if ((flags & ios_base::showpos) != 0 && v > 0) {
    char with_sign[__DAVECC_ITOA_CAPACITY(long long) + 1];
    with_sign[0] = '+';
    memcpy(with_sign + 1, buf, len);
    len += 1;
    return __write_c_string(s, f, fill, with_sign, len);
  }
  if ((flags & ios_base::showbase) != 0 && base != 10) {
    char with_base[__DAVECC_ITOA_CAPACITY(long long) + 2];
    size_t offset = 0;
    if (v >= 0) {
      with_base[offset++] = '0';
      if (base == 16) {
        with_base[offset++] = (flags & ios_base::uppercase) ? 'X' : 'x';
      }
    }
    memcpy(with_base + offset, buf, len);
    return __write_c_string(s, f, fill, with_base, offset + len);
  }
  return __write_c_string(s, f, fill, buf, len);
}

ostreambuf_iterator __davecc_classic_put_ullong(ostreambuf_iterator s, ios_base& f, char fill, unsigned long long v) {
  char buf[__DAVECC_ITOA_CAPACITY(unsigned long long)];
  ios_base::fmtflags flags = f.flags();
  int base = 10;
  if ((flags & ios_base::basefield) == ios_base::hex) {
    base = 16;
  } else if ((flags & ios_base::basefield) == ios_base::oct) {
    base = 8;
  }
  size_t len = __locale_format_ull(buf, v, base);
  if ((flags & ios_base::showbase) != 0 && base != 10) {
    char with_base[__DAVECC_ITOA_CAPACITY(unsigned long long) + 2];
    size_t offset = 0;
    with_base[offset++] = '0';
    if (base == 16) {
      with_base[offset++] = (flags & ios_base::uppercase) ? 'X' : 'x';
    }
    memcpy(with_base + offset, buf, len);
    return __write_c_string(s, f, fill, with_base, offset + len);
  }
  return __write_c_string(s, f, fill, buf, len);
}

ostreambuf_iterator __davecc_classic_put_double(ostreambuf_iterator s, ios_base& f, char fill, double v) {
  char buf[64];
  ios_base::fmtflags flags = f.flags();
  streamsize prec = f.precision();
  char* end = nullptr;
  if ((flags & ios_base::fixed) != 0) {
    end = __PrintFloatFormat(v, static_cast<int>(prec), buf, sizeof(buf));
  } else if ((flags & ios_base::scientific) != 0) {
    end = __PrintScientificFormat(v, static_cast<int>(prec), buf, sizeof(buf));
  } else {
    end = __PrintGeneralFormat(v, static_cast<int>(prec), buf, sizeof(buf));
  }
  size_t len = end != nullptr ? static_cast<size_t>(end - buf) : strlen(buf);
  return __write_c_string(s, f, fill, buf, len);
}

ostreambuf_iterator __davecc_classic_put_ldouble(ostreambuf_iterator s, ios_base& f, char fill, long double v) {
  return __davecc_classic_put_double(s, f, fill, static_cast<double>(v));
}

ostreambuf_iterator __davecc_classic_put_ptr(ostreambuf_iterator s, ios_base& f, char fill, const void* v) {
  char buf[__DAVECC_ITOA_CAPACITY(const void*)];
  size_t len = __locale_format_ptr(buf, v);
  return __write_c_string(s, f, fill, buf, len);
}


istreambuf_iterator __davecc_classic_get_bool(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, bool& v) {
  if ((f.flags() & ios_base::boolalpha) != 0) {
    const numpunct<char>& np = use_facet<numpunct<char>>(f.getloc());
    while (in != end && (*in == ' ' || *in == '\t' || *in == '\n')) {
      ++in;
    }
    basic_string<char> word;
    while (in != end && *in != ' ' && *in != '\t' && *in != '\n') {
      word += *in;
      ++in;
    }
    if (word == np.truename()) {
      v = true;
      return in;
    }
    if (word == np.falsename()) {
      v = false;
      return in;
    }
    err = ios_base::failbit;
    return in;
  }
  long long tmp = 0;
  in = __davecc_classic_get_llong(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = tmp != 0;
  }
  return in;
}

istreambuf_iterator __davecc_classic_get_long(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, long& v) {
  long long tmp = 0;
  in = __davecc_classic_get_llong(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<long>(tmp);
  }
  return in;
}

istreambuf_iterator __davecc_classic_get_ulong(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, unsigned long& v) {
  unsigned long long tmp = 0;
  in = __davecc_classic_get_ullong(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<unsigned long>(tmp);
  }
  return in;
}

istreambuf_iterator __davecc_classic_get_llong(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, long long& v) {
  err = ios_base::goodbit;
  in = __get_unsigned_integer<char, istreambuf_iterator, long long>(in, end, f,
                                                                     err, v);
  return in;
}

istreambuf_iterator __davecc_classic_get_ullong(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, unsigned long long& v) {
  err = ios_base::goodbit;
  in = __get_unsigned_integer<char, istreambuf_iterator, unsigned long long>(
      in, end, f, err, v);
  return in;
}

istreambuf_iterator __davecc_classic_get_float(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, float& v) {
  double tmp = 0;
  in = __davecc_classic_get_double(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<float>(tmp);
  }
  return in;
}

istreambuf_iterator __davecc_classic_get_double(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, double& v) {
  while (in != end && *in == ' ') {
    ++in;
  }
  char buf[64];
  size_t len = 0;
  while (in != end && len + 1 < sizeof(buf)) {
    char c = *in;
    if (c == ' ' || c == '\t' || c == '\n') {
      break;
    }
    buf[len++] = c;
    ++in;
  }
  buf[len] = 0;
  if (len == 0) {
    err = ios_base::failbit;
    return in;
  }
  char* endptr = nullptr;
  v = strtod(buf, &endptr);
  if (endptr == buf) {
    err = ios_base::failbit;
  }
  return in;
}

istreambuf_iterator __davecc_classic_get_ldouble(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, long double& v) {
  double tmp = 0;
  in = __davecc_classic_get_double(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<long double>(tmp);
  }
  return in;
}

istreambuf_iterator __davecc_classic_get_ptr(
    istreambuf_iterator in, istreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, void*& v) {
  while (in != end && *in == ' ') {
    ++in;
  }
  char buf[32];
  size_t len = 0;
  while (in != end && len + 1 < sizeof(buf)) {
    char c = *in;
    if (c == ' ' || c == '\t' || c == '\n') {
      break;
    }
    buf[len++] = c;
    ++in;
  }
  buf[len] = 0;
  if (len == 0) {
    err = ios_base::failbit;
    return in;
  }
  v = reinterpret_cast<void*>(strtoull(buf, nullptr, 16));
  return in;
}

wostreambuf_iterator __davecc_classic_wput_bool(wostreambuf_iterator s,
                                                ios_base& f, wchar_t fill,
                                                bool v) {
  const numpunct<wchar_t>& np = use_facet<numpunct<wchar_t>>(f.getloc());
  if ((f.flags() & ios_base::boolalpha) != 0) {
    basic_string<wchar_t> name = v ? np.truename() : np.falsename();
    return __write_wchars(s, f, fill, name.data(), name.size());
  }
  return __davecc_classic_wput_llong(s, f, fill, static_cast<long long>(v ? 1 : 0));
}

wostreambuf_iterator __davecc_classic_wput_long(wostreambuf_iterator s,
                                                ios_base& f, wchar_t fill,
                                                long v) {
  return __davecc_classic_wput_llong(s, f, fill, static_cast<long long>(v));
}

wostreambuf_iterator __davecc_classic_wput_ulong(wostreambuf_iterator s,
                                                 ios_base& f, wchar_t fill,
                                                 unsigned long v) {
  return __davecc_classic_wput_ullong(
      s, f, fill, static_cast<unsigned long long>(v));
}

wostreambuf_iterator __davecc_classic_wput_llong(wostreambuf_iterator s,
                                                 ios_base& f, wchar_t fill,
                                                 long long v) {
  char buf[__DAVECC_ITOA_CAPACITY(long long)];
  ios_base::fmtflags flags = f.flags();
  int base = 10;
  if ((flags & ios_base::basefield) == ios_base::hex) {
    base = 16;
  } else if ((flags & ios_base::basefield) == ios_base::oct) {
    base = 8;
  }
  size_t len = __locale_format_ll(buf, v, base);
  if ((flags & ios_base::showpos) != 0 && v > 0) {
    char with_sign[__DAVECC_ITOA_CAPACITY(long long) + 1];
    with_sign[0] = '+';
    memcpy(with_sign + 1, buf, len);
    len += 1;
    return __write_wc_string(s, f, fill, with_sign, len);
  }
  if ((flags & ios_base::showbase) != 0 && base != 10) {
    char with_base[__DAVECC_ITOA_CAPACITY(long long) + 2];
    size_t offset = 0;
    if (v >= 0) {
      with_base[offset++] = '0';
      if (base == 16) {
        with_base[offset++] = (flags & ios_base::uppercase) ? 'X' : 'x';
      }
    }
    memcpy(with_base + offset, buf, len);
    return __write_wc_string(s, f, fill, with_base, offset + len);
  }
  return __write_wc_string(s, f, fill, buf, len);
}

wostreambuf_iterator __davecc_classic_wput_ullong(wostreambuf_iterator s,
                                                  ios_base& f, wchar_t fill,
                                                  unsigned long long v) {
  char buf[__DAVECC_ITOA_CAPACITY(unsigned long long)];
  ios_base::fmtflags flags = f.flags();
  int base = 10;
  if ((flags & ios_base::basefield) == ios_base::hex) {
    base = 16;
  } else if ((flags & ios_base::basefield) == ios_base::oct) {
    base = 8;
  }
  size_t len = __locale_format_ull(buf, v, base);
  if ((flags & ios_base::showbase) != 0 && base != 10) {
    char with_base[__DAVECC_ITOA_CAPACITY(unsigned long long) + 2];
    size_t offset = 0;
    with_base[offset++] = '0';
    if (base == 16) {
      with_base[offset++] = (flags & ios_base::uppercase) ? 'X' : 'x';
    }
    memcpy(with_base + offset, buf, len);
    return __write_wc_string(s, f, fill, with_base, offset + len);
  }
  return __write_wc_string(s, f, fill, buf, len);
}

wostreambuf_iterator __davecc_classic_wput_double(wostreambuf_iterator s,
                                                ios_base& f, wchar_t fill,
                                                double v) {
  char buf[64];
  ios_base::fmtflags flags = f.flags();
  streamsize prec = f.precision();
  char* end = nullptr;
  if ((flags & ios_base::fixed) != 0) {
    end = __PrintFloatFormat(v, static_cast<int>(prec), buf, sizeof(buf));
  } else if ((flags & ios_base::scientific) != 0) {
    end = __PrintScientificFormat(v, static_cast<int>(prec), buf, sizeof(buf));
  } else {
    end = __PrintGeneralFormat(v, static_cast<int>(prec), buf, sizeof(buf));
  }
  size_t len = end != nullptr ? static_cast<size_t>(end - buf) : strlen(buf);
  return __write_wc_string(s, f, fill, buf, len);
}

wostreambuf_iterator __davecc_classic_wput_ldouble(wostreambuf_iterator s,
                                                   ios_base& f, wchar_t fill,
                                                   long double v) {
  return __davecc_classic_wput_double(s, f, fill, static_cast<double>(v));
}

wostreambuf_iterator __davecc_classic_wput_ptr(wostreambuf_iterator s,
                                               ios_base& f, wchar_t fill,
                                               const void* v) {
  char buf[__DAVECC_ITOA_CAPACITY(const void*)];
  size_t len = __locale_format_ptr(buf, v);
  return __write_wc_string(s, f, fill, buf, len);
}

wistreambuf_iterator __davecc_classic_wget_bool(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, bool& v) {
  if ((f.flags() & ios_base::boolalpha) != 0) {
    const numpunct<wchar_t>& np = use_facet<numpunct<wchar_t>>(f.getloc());
    while (in != end && (*in == L' ' || *in == L'\t' || *in == L'\n')) {
      ++in;
    }
    basic_string<wchar_t> word;
    while (in != end && *in != L' ' && *in != L'\t' && *in != L'\n') {
      word += *in;
      ++in;
    }
    if (word == np.truename()) {
      v = true;
      return in;
    }
    if (word == np.falsename()) {
      v = false;
      return in;
    }
    err = ios_base::failbit;
    return in;
  }
  long long tmp = 0;
  in = __davecc_classic_wget_llong(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = tmp != 0;
  }
  return in;
}

wistreambuf_iterator __davecc_classic_wget_long(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, long& v) {
  long long tmp = 0;
  in = __davecc_classic_wget_llong(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<long>(tmp);
  }
  return in;
}

wistreambuf_iterator __davecc_classic_wget_ulong(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, unsigned long& v) {
  unsigned long long tmp = 0;
  in = __davecc_classic_wget_ullong(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<unsigned long>(tmp);
  }
  return in;
}

wistreambuf_iterator __davecc_classic_wget_llong(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, long long& v) {
  err = ios_base::goodbit;
  in = __get_unsigned_integer<wchar_t, wistreambuf_iterator, long long>(
      in, end, f, err, v);
  return in;
}

wistreambuf_iterator __davecc_classic_wget_ullong(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, unsigned long long& v) {
  err = ios_base::goodbit;
  in = __get_unsigned_integer<wchar_t, wistreambuf_iterator,
                              unsigned long long>(in, end, f, err, v);
  return in;
}

wistreambuf_iterator __davecc_classic_wget_float(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, float& v) {
  double tmp = 0;
  in = __davecc_classic_wget_double(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<float>(tmp);
  }
  return in;
}

wistreambuf_iterator __davecc_classic_wget_double(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, double& v) {
  while (in != end && *in == L' ') {
    ++in;
  }
  char buf[64];
  size_t len = 0;
  while (in != end && len + 1 < sizeof(buf)) {
    wchar_t wc = *in;
    if (wc == L' ' || wc == L'\t' || wc == L'\n') {
      break;
    }
    if (wc > 127) {
      err = ios_base::failbit;
      return in;
    }
    buf[len++] = static_cast<char>(wc);
    ++in;
  }
  buf[len] = 0;
  if (len == 0) {
    err = ios_base::failbit;
    return in;
  }
  char* endptr = nullptr;
  v = strtod(buf, &endptr);
  if (endptr == buf) {
    err = ios_base::failbit;
  }
  return in;
}

wistreambuf_iterator __davecc_classic_wget_ldouble(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, long double& v) {
  double tmp = 0;
  in = __davecc_classic_wget_double(in, end, f, err, tmp);
  if ((err & ios_base::failbit) == 0) {
    v = static_cast<long double>(tmp);
  }
  return in;
}

wistreambuf_iterator __davecc_classic_wget_ptr(
    wistreambuf_iterator in, wistreambuf_iterator end, ios_base& f,
    ios_base::iostate& err, void*& v) {
  while (in != end && *in == L' ') {
    ++in;
  }
  char buf[32];
  size_t len = 0;
  while (in != end && len + 1 < sizeof(buf)) {
    wchar_t wc = *in;
    if (wc == L' ' || wc == L'\t' || wc == L'\n') {
      break;
    }
    if (wc > 127) {
      err = ios_base::failbit;
      return in;
    }
    buf[len++] = static_cast<char>(wc);
    ++in;
  }
  buf[len] = 0;
  if (len == 0) {
    err = ios_base::failbit;
    return in;
  }
  v = reinterpret_cast<void*>(strtoull(buf, nullptr, 16));
  return in;
}

namespace {

class __classic_numpunct_char : public numpunct<char> {
 public:
  __classic_numpunct_char() : numpunct<char>() {}
};

class __classic_numpunct_wchar : public numpunct<wchar_t> {
 public:
  __classic_numpunct_wchar() : numpunct<wchar_t>() {}
};

void __install_classic_facets(__locale_impl* impl) {
  __install_facet<ctype<char>>(impl, __make_facet<ctype<char>>());
  __install_facet<ctype<wchar_t>>(impl, __make_facet<ctype<wchar_t>>());
  __install_facet<numpunct<char>>(impl, __make_facet<__classic_numpunct_char>());
  __install_facet<numpunct<wchar_t>>(
      impl, __make_facet<__classic_numpunct_wchar>());
  __install_facet<__davecc_num_put_char>(impl, __make_facet<__davecc_num_put_char>());
  __install_facet<__davecc_num_get_char>(impl, __make_facet<__davecc_num_get_char>());
  __install_facet<__davecc_num_put_wchar>(impl, __make_facet<__davecc_num_put_wchar>());
  __install_facet<__davecc_num_get_wchar>(impl, __make_facet<__davecc_num_get_wchar>());
}

}  // namespace

__locale_impl* locale::__make_classic() {
  static __locale_impl* classic = nullptr;
  if (classic == nullptr) {
    classic = new __locale_impl("C");
    __install_classic_facets(classic);
  }
  return classic;
}

__locale_impl* locale::__make_named(const char* name) {
  if (!__locale_detail::__is_classic_name(name)) {
    __locale_detail::__throw_runtime("unsupported locale name");
  }
  return __make_classic();
}

__locale_impl* locale::__combine_impl(const __locale_impl* base,
                                      const __locale_impl* add,
                                      locale::category cat) {
  if (base == nullptr) {
    return add != nullptr ? new __locale_impl(*add) : __make_classic();
  }
  return base->__combine_with(add, cat);
}

static __locale_impl*& __global_locale_impl() {
  static __locale_impl* impl = nullptr;
  if (impl == nullptr) {
    impl = locale::__make_classic();
    ++impl->__refs;
  }
  return impl;
}

locale::locale() noexcept : __impl_(__global_locale_impl()) {
  __impl_->__refs++;
}

locale::locale(const locale& other) noexcept : __impl_(other.__impl_) {
  if (__impl_ != nullptr) {
    __impl_->__refs++;
  }
}

locale::locale(const char* std_name) : __impl_(__make_named(std_name)) {
  __impl_->__refs++;
}

locale::locale(const locale& other, const char* std_name, locale::category cat)
    : __impl_(__combine_impl(other.__impl_, __make_named(std_name), cat)) {
  if (__impl_ != nullptr) {
    __impl_->__refs++;
  }
}

locale::locale(const locale& other, const locale& add, locale::category cat)
    : __impl_(__combine_impl(other.__impl_, add.__impl_, cat)) {
  if (__impl_ != nullptr) {
    __impl_->__refs++;
  }
}

locale::~locale() {
  if (__impl_ != nullptr && --__impl_->__refs == 0) {
    delete __impl_;
  }
}

const locale& locale::operator=(const locale& other) noexcept {
  if (this != &other) {
    if (__impl_ != nullptr && --__impl_->__refs == 0) {
      delete __impl_;
    }
    __impl_ = other.__impl_;
    if (__impl_ != nullptr) {
      __impl_->__refs++;
    }
  }
  return *this;
}

string locale::name() const {
  return __impl_ != nullptr ? string(__impl_->__name) : string("C");
}

bool locale::operator==(const locale& other) const {
  return __impl_ == other.__impl_;
}

locale locale::global(const locale& loc) {
  __locale_impl* old_impl = __global_locale_impl();
  if (loc.__impl_ != nullptr) {
    ++loc.__impl_->__refs;
  }
  __global_locale_impl() = loc.__impl_;
  return locale(old_impl, locale::__from_impl());
}

const locale& locale::classic() {
  static locale* inst = nullptr;
  if (inst == nullptr) {
    const char* c_name = "C";
    inst = new locale(c_name);
  }
  return *inst;
}

namespace {

struct __locale_binding {
  const void* object;
  locale value;
  __locale_binding* next;

  __locale_binding(const void* key, const locale& loc, __locale_binding* tail)
      : object(key), value(loc), next(tail) {}
};

__locale_binding*& __ios_locale_bindings() {
  static __locale_binding* bindings = nullptr;
  return bindings;
}

__locale_binding*& __streambuf_locale_bindings() {
  static __locale_binding* bindings = nullptr;
  return bindings;
}

locale __binding_get(__locale_binding* bindings, const void* object) {
  for (__locale_binding* current = bindings; current != nullptr;
       current = current->next) {
    if (current->object == object) {
      return current->value;
    }
  }
  return locale();
}

void __binding_set(__locale_binding*& bindings, const void* object,
                   const locale& value) {
  for (__locale_binding* current = bindings; current != nullptr;
       current = current->next) {
    if (current->object == object) {
      current->value = value;
      return;
    }
  }
  bindings = new __locale_binding(object, value, bindings);
}

void __binding_erase(__locale_binding*& bindings, const void* object) {
  __locale_binding** link = &bindings;
  while (*link != nullptr) {
    if ((*link)->object == object) {
      __locale_binding* removed = *link;
      *link = removed->next;
      delete removed;
      return;
    }
    link = &(*link)->next;
  }
}

}  // namespace

locale ios_base::imbue(const locale& loc) {
  locale old = getloc();
  for (int i = 0; i < __callback_count_; ++i) {
    if (__callbacks_[i] != nullptr) {
      __callbacks_[i](ios_base::imbue_event, *this, __callback_indices_[i]);
    }
  }
  __binding_set(__ios_locale_bindings(), this, loc);
  return old;
}

locale ios_base::getloc() const {
  return __binding_get(__ios_locale_bindings(), this);
}

void ios_base::register_callback(ios_base::event_callback fn, int index) {
  if (__callback_count_ < __max_callbacks_) {
    __callbacks_[__callback_count_] = fn;
    __callback_indices_[__callback_count_] = index;
    ++__callback_count_;
  }
}

locale basic_streambuf<char, char_traits<char>>::pubimbue(const locale& loc) {
  return imbue(loc);
}

locale basic_streambuf<char, char_traits<char>>::getloc() const {
  return __binding_get(__streambuf_locale_bindings(), this);
}

locale basic_streambuf<char, char_traits<char>>::imbue(const locale& loc) {
  locale old = getloc();
  __binding_set(__streambuf_locale_bindings(), this, loc);
  return old;
}

locale basic_streambuf<wchar_t, char_traits<wchar_t>>::pubimbue(
    const locale& loc) {
  return imbue(loc);
}

locale basic_streambuf<wchar_t, char_traits<wchar_t>>::getloc() const {
  return __binding_get(__streambuf_locale_bindings(), this);
}

locale basic_streambuf<wchar_t, char_traits<wchar_t>>::imbue(
    const locale& loc) {
  locale old = getloc();
  __binding_set(__streambuf_locale_bindings(), this, loc);
  return old;
}

}  // namespace std
