#include <locale>

#include <__itoa.h>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <istream>
#include <streambuf>

// Numeric facet formatting and parsing are kept separate from the locale core
// so streambuf vtables do not retain the entire numeric implementation.

extern "C" char* __PrintFloatFormat(double, int, char*, size_t);
extern "C" char* __PrintScientificFormat(double, int, char*, size_t);
extern "C" char* __PrintGeneralFormat(double, int, char*, size_t);

namespace std {

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

}  // namespace std
