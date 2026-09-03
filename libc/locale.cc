#include <locale>

#include <cctype>
#include <ios>
#include <istream>
#include <streambuf>
#include <typeinfo>
#include <vector>

// Locale core: classic facet registry and iostream locale hooks.

#ifndef __cpp_exceptions
extern "C" void __davecc_raise_bad_cast(void);
#endif

namespace std {

struct __locale_impl {
  mutable size_t __refs;
  const char* __name;
  vector<locale::facet*> __facets;

  explicit __locale_impl(const char* name);
  __locale_impl(const __locale_impl& other);
  ~__locale_impl();

  locale::facet* __get(size_t index) const {
    return index < __facets.size() ? __facets[index] : nullptr;
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

__locale_impl::__locale_impl(const char* name)
    : __refs(0), __name(name != nullptr ? name : "C"), __facets() {}

__locale_impl::__locale_impl(const __locale_impl& other)
    : __refs(0), __name(other.__name), __facets(other.__facets) {
  for (size_t i = 0; i < __facets.size(); ++i) {
    if (__facets[i] != nullptr) {
      __facets[i]->__add_ref();
    }
  }
}

__locale_impl::~__locale_impl() {
  for (size_t i = 0; i < __facets.size(); ++i) {
    if (__facets[i] != nullptr) {
      __facets[i]->__release();
    }
  }
}

void __locale_impl::__install(locale::facet* facet, size_t index) {
  if (facet == nullptr) {
    return;
  }
  if (index >= __facets.size()) {
    __facets.resize(index + 1, static_cast<locale::facet*>(nullptr));
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

  if ((cat & locale::collate) != 0) {
    locale::facet* facet = source->__get(collate<char>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, collate<char>::id.__index());
    }
    facet = source->__get(collate<wchar_t>::id.__index());
    if (facet != nullptr) {
      result->__install(facet, collate<wchar_t>::id.__index());
    }
  }
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

template class collate<char>;
template class collate<wchar_t>;

locale::id ctype<char>::id;
locale::id ctype<wchar_t>::id;
locale::id collate<char>::id;
locale::id collate<wchar_t>::id;
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
  __install_facet<collate<char>>(impl, __make_facet<collate<char>>());
  __install_facet<collate<wchar_t>>(impl, __make_facet<collate<wchar_t>>());
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

__locale_impl* locale::__replace_facet_impl(const __locale_impl* base,
                                            locale::facet* replacement,
                                            locale::id& facet_id) {
  if (replacement == nullptr) {
    __locale_impl* result = const_cast<__locale_impl*>(base);
    if (result != nullptr) {
      ++result->__refs;
    }
    return result;
  }
  __locale_impl* result =
      base != nullptr ? new __locale_impl(*base) : new __locale_impl("C");
  result->__name = "*";
#ifdef __cpp_exceptions
  try {
    result->__install(replacement, facet_id.__index());
  } catch (...) {
    delete result;
    delete replacement;
    throw;
  }
#else
  result->__install(replacement, facet_id.__index());
#endif
  result->__refs = 1;
  return result;
}

__locale_impl* locale::__combine_facet_impl(const __locale_impl* base,
                                            const __locale_impl* add,
                                            locale::id& facet_id) {
  locale::facet* replacement =
      add != nullptr ? add->__get(facet_id.__index()) : nullptr;
  if (replacement == nullptr) {
    __locale_detail::__throw_bad_cast();
  }
  __locale_impl* result =
      base != nullptr ? new __locale_impl(*base) : new __locale_impl("C");
  result->__name = "*";
#ifdef __cpp_exceptions
  try {
    result->__install(replacement, facet_id.__index());
  } catch (...) {
    delete result;
    throw;
  }
#else
  result->__install(replacement, facet_id.__index());
#endif
  result->__refs = 1;
  return result;
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
