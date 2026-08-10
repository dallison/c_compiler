#include <filesystem>

#include <cerrno>
#include <cstdlib>
#include <syscall.h>

namespace std {
namespace filesystem {
namespace __filesystem_detail {

struct __wire_status {
  uint64_t device;
  uint64_t inode;
  uint64_t size;
  uint64_t hard_link_count;
  int64_t access_time_ns;
  int64_t modification_time_ns;
  int64_t status_change_time_ns;
  uint32_t mode;
  uint32_t reserved;
};

struct __wire_directory_entry {
  uint32_t type;
  uint32_t reserved;
  char name[1024];
};

struct __wire_space {
  uint64_t capacity;
  uint64_t free;
  uint64_t available;
};

static int64_t __status_call(const char* value, bool follow,
                             __wire_status* result) {
  return syscall(SYS_FS_STATUS, value, follow ? 1 : 0, result);
}

static int64_t __call1(int operation, const void* first) {
  return syscall(operation, first);
}

static int64_t __call2(int operation, const void* first, const void* second) {
  return syscall(operation, first, second);
}

static int64_t __call3(int operation, const void* first, const void* second,
                       const void* third) {
  return syscall(operation, first, second, third);
}

static bool __error(error_code& error, int64_t result) {
  if (result >= 0) {
    error.clear();
    return false;
  }
  error.assign(static_cast<int>(-result), generic_category());
  return true;
}

[[noreturn]] static void __throw_error(const char* operation, const path& first,
                                       const error_code& error) {
  __DAVECC_THROW(filesystem_error(operation, first, error));
}

[[noreturn]] static void __throw_error(const char* operation, const path& first,
                                       const path& second,
                                       const error_code& error) {
  __DAVECC_THROW(filesystem_error(operation, first, second, error));
}

static file_type __mode_type(uint32_t mode) {
  switch (mode & 0170000) {
    case 0100000: return file_type::regular;
    case 0040000: return file_type::directory;
    case 0120000: return file_type::symlink;
    case 0060000: return file_type::block;
    case 0020000: return file_type::character;
    case 0010000: return file_type::fifo;
    case 0140000: return file_type::socket;
    default: return file_type::unknown;
  }
}

static file_status __status(const path& value, bool follow, error_code& error,
                            __wire_status* raw = nullptr) {
  __wire_status local;
  __wire_status* output = raw == nullptr ? &local : raw;
  int64_t result = __status_call(value.c_str(), follow, output);
  if (result == -ENOENT || result == -ENOTDIR) {
    error.clear();
    return file_status(file_type::not_found);
  }
  if (__error(error, result)) {
    return file_status(file_type::none);
  }
  return file_status(__mode_type(output->mode),
                     static_cast<perms>(output->mode & 07777));
}

class __directory_stream {
 public:
  __directory_stream(const path& value, directory_options options,
                     error_code& error)
      : __directory(value), __options(options), __handle(-1) {
    int64_t result = __call1(SYS_FS_OPEN_DIRECTORY, value.c_str());
    if (__error(error, result)) return;
    __handle = static_cast<int>(result);
    __advance(error);
  }

  ~__directory_stream() {
    if (__handle >= 0) {
      __call1(SYS_FS_CLOSE_DIRECTORY,
              reinterpret_cast<const void*>(static_cast<intptr_t>(__handle)));
    }
  }

  const directory_entry& current() const { return __current; }
  bool at_end() const { return __handle < 0; }

  void __advance(error_code& error) {
    if (__handle < 0) {
      error.clear();
      return;
    }
    __wire_directory_entry entry;
    int64_t result = syscall(SYS_FS_READ_DIRECTORY, __handle, &entry);
    if (__error(error, result)) {
      __close();
      return;
    }
    if (result == 0) {
      __close();
      return;
    }
    __current.assign(__directory / path(entry.name));
  }

 private:
  void __close() {
    if (__handle >= 0) {
      syscall(SYS_FS_CLOSE_DIRECTORY, __handle);
      __handle = -1;
    }
  }

  path __directory;
  directory_options __options;
  int __handle;
  directory_entry __current;
};

class __recursive_directory_stream {
 public:
  __recursive_directory_stream(const path& value, directory_options options,
                               error_code& error)
      : __options(options), __recursion_pending(true) {
    directory_iterator first(value, options, error);
    if (!error && first != directory_iterator()) {
      __levels.push_back(first);
    }
  }

  const directory_entry& current() const { return *__levels.back(); }
  bool at_end() const { return __levels.empty(); }
  int depth() const { return static_cast<int>(__levels.size()) - 1; }
  directory_options options() const { return __options; }
  bool recursion_pending() const { return __recursion_pending; }
  void disable_recursion_pending() { __recursion_pending = false; }

  void advance(error_code& error) {
    if (__levels.empty()) {
      error.clear();
      return;
    }
    if (__recursion_pending) {
      bool descend = current().is_directory(error);
      if (error) return;
      if (descend &&
          (((__options & directory_options::follow_directory_symlink) !=
            directory_options::none) ||
           !current().is_symlink(error))) {
        if (error) return;
        directory_iterator child(current().path(), __options, error);
        if (error &&
            (__options & directory_options::skip_permission_denied) !=
                directory_options::none &&
            error.value() == EACCES) {
          error.clear();
        } else if (error) {
          return;
        } else if (child != directory_iterator()) {
          __levels.push_back(child);
          __recursion_pending = true;
          return;
        }
      }
    }
    __recursion_pending = true;
    __levels.back().increment(error);
    if (error) return;
    while (!__levels.empty() &&
           __levels.back() == directory_iterator()) {
      __levels.pop_back();
      if (!__levels.empty()) {
        __levels.back().increment(error);
        if (error) return;
      }
    }
  }

  void pop(error_code& error) {
    if (__levels.empty()) {
      error.clear();
      return;
    }
    __levels.pop_back();
    if (!__levels.empty()) {
      __levels.back().increment(error);
      if (error) return;
      while (!__levels.empty() &&
             __levels.back() == directory_iterator()) {
        __levels.pop_back();
        if (!__levels.empty()) {
          __levels.back().increment(error);
          if (error) return;
        }
      }
    }
    error.clear();
    __recursion_pending = true;
  }

 private:
  directory_options __options;
  bool __recursion_pending;
  vector<directory_iterator> __levels;
};

}  // namespace __filesystem_detail

__file_clock::time_point __file_clock::now() noexcept {
  return time_point(duration(
      chrono::duration_cast<chrono::nanoseconds>(
          chrono::system_clock::now().time_since_epoch())
          .count()));
}

bool status_known(file_status value) noexcept {
  return value.type() != file_type::none;
}
bool exists(file_status value) noexcept {
  return status_known(value) && value.type() != file_type::not_found;
}
bool is_block_file(file_status value) noexcept {
  return value.type() == file_type::block;
}
bool is_character_file(file_status value) noexcept {
  return value.type() == file_type::character;
}
bool is_directory(file_status value) noexcept {
  return value.type() == file_type::directory;
}
bool is_fifo(file_status value) noexcept {
  return value.type() == file_type::fifo;
}
bool is_regular_file(file_status value) noexcept {
  return value.type() == file_type::regular;
}
bool is_socket(file_status value) noexcept {
  return value.type() == file_type::socket;
}
bool is_symlink(file_status value) noexcept {
  return value.type() == file_type::symlink;
}
bool is_other(file_status value) noexcept {
  return exists(value) && !is_regular_file(value) && !is_directory(value) &&
         !is_symlink(value);
}

directory_iterator::directory_iterator(const path& value)
    : directory_iterator(value, directory_options::none) {}
directory_iterator::directory_iterator(const path& value,
                                       directory_options options) {
  error_code error;
  __stream = make_shared<__filesystem_detail::__directory_stream>(
      value, options, error);
  if (error) __filesystem_detail::__throw_error("directory_iterator", value,
                                                error);
  if (__stream->at_end()) __stream.reset();
}
directory_iterator::directory_iterator(const path& value, error_code& error)
    : directory_iterator(value, directory_options::none, error) {}
directory_iterator::directory_iterator(const path& value,
                                       directory_options options,
                                       error_code& error) {
  __stream = make_shared<__filesystem_detail::__directory_stream>(
      value, options, error);
  if (error || __stream->at_end()) __stream.reset();
}
directory_iterator::reference directory_iterator::operator*() const {
  return __stream->current();
}
directory_iterator::pointer directory_iterator::operator->() const {
  return &__stream->current();
}
directory_iterator& directory_iterator::operator++() {
  error_code error;
  increment(error);
  if (error) {
    __filesystem_detail::__throw_error("directory_iterator::increment",
                                        path(), error);
  }
  return *this;
}
directory_iterator& directory_iterator::increment(error_code& error) {
  if (!__stream) {
    error.clear();
    return *this;
  }
  __stream->__advance(error);
  if (error || __stream->at_end()) __stream.reset();
  return *this;
}

recursive_directory_iterator::recursive_directory_iterator(const path& value)
    : recursive_directory_iterator(value, directory_options::none) {}
recursive_directory_iterator::recursive_directory_iterator(
    const path& value, directory_options options) {
  error_code error;
  __stream =
      make_shared<__filesystem_detail::__recursive_directory_stream>(
          value, options, error);
  if (error) __filesystem_detail::__throw_error(
      "recursive_directory_iterator", value, error);
  if (__stream->at_end()) __stream.reset();
}
recursive_directory_iterator::recursive_directory_iterator(
    const path& value, error_code& error)
    : recursive_directory_iterator(value, directory_options::none, error) {}
recursive_directory_iterator::recursive_directory_iterator(
    const path& value, directory_options options, error_code& error) {
  __stream =
      make_shared<__filesystem_detail::__recursive_directory_stream>(
          value, options, error);
  if (error || __stream->at_end()) __stream.reset();
}
recursive_directory_iterator::reference
recursive_directory_iterator::operator*() const {
  return __stream->current();
}
recursive_directory_iterator::pointer
recursive_directory_iterator::operator->() const {
  return &__stream->current();
}
directory_options recursive_directory_iterator::options() const {
  return __stream ? __stream->options() : directory_options::none;
}
int recursive_directory_iterator::depth() const {
  return __stream ? __stream->depth() : 0;
}
bool recursive_directory_iterator::recursion_pending() const {
  return __stream && __stream->recursion_pending();
}
void recursive_directory_iterator::disable_recursion_pending() {
  if (__stream) __stream->disable_recursion_pending();
}
void recursive_directory_iterator::pop() {
  error_code error;
  pop(error);
  if (error) __filesystem_detail::__throw_error(
      "recursive_directory_iterator::pop", path(), error);
}
void recursive_directory_iterator::pop(error_code& error) {
  if (!__stream) {
    error.clear();
    return;
  }
  __stream->pop(error);
  if (error || __stream->at_end()) __stream.reset();
}
recursive_directory_iterator& recursive_directory_iterator::operator++() {
  error_code error;
  increment(error);
  if (error) __filesystem_detail::__throw_error(
      "recursive_directory_iterator::increment", path(), error);
  return *this;
}
recursive_directory_iterator& recursive_directory_iterator::increment(
    error_code& error) {
  if (!__stream) {
    error.clear();
    return *this;
  }
  __stream->advance(error);
  if (error || __stream->at_end()) __stream.reset();
  return *this;
}

directory_iterator begin(directory_iterator iterator) noexcept {
  return iterator;
}
directory_iterator end(directory_iterator) noexcept {
  return directory_iterator();
}
recursive_directory_iterator begin(
    recursive_directory_iterator iterator) noexcept {
  return iterator;
}
recursive_directory_iterator end(recursive_directory_iterator) noexcept {
  return recursive_directory_iterator();
}

file_status status(const path& value, error_code& error) noexcept {
  return __filesystem_detail::__status(value, true, error);
}
file_status status(const path& value) {
  error_code error;
  file_status result = status(value, error);
  if (error) __filesystem_detail::__throw_error("status", value, error);
  return result;
}
file_status symlink_status(const path& value, error_code& error) noexcept {
  return __filesystem_detail::__status(value, false, error);
}
file_status symlink_status(const path& value) {
  error_code error;
  file_status result = symlink_status(value, error);
  if (error) __filesystem_detail::__throw_error("symlink_status", value, error);
  return result;
}

bool exists(const path& value, error_code& error) noexcept {
  return filesystem::exists(status(value, error));
}
bool exists(const path& value) { return filesystem::exists(status(value)); }

#define DAVECC_FS_PATH_PREDICATE(name)                                      \
  bool name(const path& value, error_code& error) noexcept {                \
    return filesystem::name(status(value, error));                           \
  }                                                                          \
  bool name(const path& value) { return filesystem::name(status(value)); }

DAVECC_FS_PATH_PREDICATE(is_block_file)
DAVECC_FS_PATH_PREDICATE(is_character_file)
DAVECC_FS_PATH_PREDICATE(is_directory)
DAVECC_FS_PATH_PREDICATE(is_fifo)
DAVECC_FS_PATH_PREDICATE(is_other)
DAVECC_FS_PATH_PREDICATE(is_regular_file)
DAVECC_FS_PATH_PREDICATE(is_socket)

#undef DAVECC_FS_PATH_PREDICATE

bool is_symlink(const path& value, error_code& error) noexcept {
  return filesystem::is_symlink(symlink_status(value, error));
}
bool is_symlink(const path& value) {
  return filesystem::is_symlink(symlink_status(value));
}

uintmax_t file_size(const path& value, error_code& error) noexcept {
  __filesystem_detail::__wire_status raw;
  file_status value_status =
      __filesystem_detail::__status(value, true, error, &raw);
  if (error || !is_regular_file(value_status)) {
    if (!error) error.assign(EISDIR, generic_category());
    return static_cast<uintmax_t>(-1);
  }
  return static_cast<uintmax_t>(raw.size);
}
uintmax_t file_size(const path& value) {
  error_code error;
  uintmax_t result = file_size(value, error);
  if (error) __filesystem_detail::__throw_error("file_size", value, error);
  return result;
}

uintmax_t hard_link_count(const path& value, error_code& error) noexcept {
  __filesystem_detail::__wire_status raw;
  __filesystem_detail::__status(value, true, error, &raw);
  return error ? static_cast<uintmax_t>(-1)
               : static_cast<uintmax_t>(raw.hard_link_count);
}
uintmax_t hard_link_count(const path& value) {
  error_code error;
  uintmax_t result = hard_link_count(value, error);
  if (error)
    __filesystem_detail::__throw_error("hard_link_count", value, error);
  return result;
}

file_time_type last_write_time(const path& value, error_code& error) noexcept {
  __filesystem_detail::__wire_status raw;
  __filesystem_detail::__status(value, true, error, &raw);
  return error ? file_time_type::min()
               : file_time_type(chrono::nanoseconds(raw.modification_time_ns));
}
file_time_type last_write_time(const path& value) {
  error_code error;
  file_time_type result = last_write_time(value, error);
  if (error)
    __filesystem_detail::__throw_error("last_write_time", value, error);
  return result;
}
void last_write_time(const path& value, file_time_type time,
                     error_code& error) noexcept {
  int64_t nanoseconds =
      chrono::duration_cast<chrono::nanoseconds>(time.time_since_epoch())
          .count();
  __filesystem_detail::__error(
      error, __filesystem_detail::__call2(
                 SYS_FS_SET_MODIFICATION_TIME, value.c_str(), &nanoseconds));
}
void last_write_time(const path& value, file_time_type time) {
  error_code error;
  last_write_time(value, time, error);
  if (error)
    __filesystem_detail::__throw_error("last_write_time", value, error);
}

path current_path(error_code& error) {
  size_t capacity = 256;
  for (;;) {
    vector<char> buffer(capacity);
    int64_t result = syscall(SYS_FS_CURRENT_PATH, buffer.data(), capacity);
    if (result == -ERANGE) {
      capacity *= 2;
      continue;
    }
    if (__filesystem_detail::__error(error, result)) return path();
    return path(buffer.data());
  }
}
path current_path() {
  error_code error;
  path result = current_path(error);
  if (error) __filesystem_detail::__throw_error("current_path", path(), error);
  return result;
}
void current_path(const path& value, error_code& error) noexcept {
  __filesystem_detail::__error(
      error, __filesystem_detail::__call1(SYS_FS_SET_CURRENT_PATH,
                                          value.c_str()));
}
void current_path(const path& value) {
  error_code error;
  current_path(value, error);
  if (error) __filesystem_detail::__throw_error("current_path", value, error);
}

path absolute(const path& value, error_code& error) {
  if (value.is_absolute()) {
    error.clear();
    return value;
  }
  path base = current_path(error);
  return error ? path() : base / value;
}
path absolute(const path& value) {
  error_code error;
  path result = absolute(value, error);
  if (error) __filesystem_detail::__throw_error("absolute", value, error);
  return result;
}

path canonical(const path& value, error_code& error) {
  size_t capacity = 256;
  for (;;) {
    vector<char> buffer(capacity);
    int64_t result =
        syscall(SYS_FS_CANONICAL, value.c_str(), buffer.data(), capacity);
    if (result == -ERANGE) {
      capacity *= 2;
      continue;
    }
    if (__filesystem_detail::__error(error, result)) return path();
    return path(buffer.data());
  }
}
path canonical(const path& value) {
  error_code error;
  path result = canonical(value, error);
  if (error) __filesystem_detail::__throw_error("canonical", value, error);
  return result;
}

path weakly_canonical(const path& value, error_code& error) {
  path probe = absolute(value, error).lexically_normal();
  if (error) return path();
  vector<path> missing;
  while (!probe.empty() && !exists(probe, error)) {
    if (error) return path();
    missing.push_back(probe.filename());
    path parent = probe.parent_path();
    if (parent == probe) break;
    probe = parent;
  }
  path result = canonical(probe, error);
  if (error) return path();
  for (size_t i = missing.size(); i > 0; --i) result /= missing[i - 1];
  return result.lexically_normal();
}
path weakly_canonical(const path& value) {
  error_code error;
  path result = weakly_canonical(value, error);
  if (error)
    __filesystem_detail::__throw_error("weakly_canonical", value, error);
  return result;
}

path relative(const path& value, const path& base, error_code& error) {
  path left = weakly_canonical(value, error);
  if (error) return path();
  path right = weakly_canonical(base, error);
  return error ? path() : left.lexically_relative(right);
}
path relative(const path& value, error_code& error) {
  path base = current_path(error);
  return error ? path() : relative(value, base, error);
}
path relative(const path& value, const path& base) {
  error_code error;
  path result = relative(value, base, error);
  if (error)
    __filesystem_detail::__throw_error("relative", value, base, error);
  return result;
}
path proximate(const path& value, const path& base, error_code& error) {
  path result = relative(value, base, error);
  return !error && result.empty() ? value : result;
}
path proximate(const path& value, error_code& error) {
  path base = current_path(error);
  return error ? path() : proximate(value, base, error);
}
path proximate(const path& value, const path& base) {
  error_code error;
  path result = proximate(value, base, error);
  if (error)
    __filesystem_detail::__throw_error("proximate", value, base, error);
  return result;
}

bool create_directory(const path& value, error_code& error) noexcept {
  int64_t result = syscall(SYS_FS_CREATE_DIRECTORY, value.c_str(), 0777);
  if (result == -EEXIST) {
    bool result_is_directory = is_directory(value, error);
    return error ? false : !result_is_directory ? false : false;
  }
  if (__filesystem_detail::__error(error, result)) return false;
  return true;
}
bool create_directory(const path& value) {
  error_code error;
  bool result = create_directory(value, error);
  if (error)
    __filesystem_detail::__throw_error("create_directory", value, error);
  return result;
}
bool create_directory(const path& value, const path& attributes,
                      error_code& error) noexcept {
  file_status attributes_status = status(attributes, error);
  if (error) return false;
  int64_t result = syscall(
      SYS_FS_CREATE_DIRECTORY, value.c_str(),
      static_cast<unsigned>(attributes_status.permissions()) & 07777);
  if (result == -EEXIST) {
    error.clear();
    return false;
  }
  return !__filesystem_detail::__error(error, result);
}
bool create_directory(const path& value, const path& attributes) {
  error_code error;
  bool result = create_directory(value, attributes, error);
  if (error) __filesystem_detail::__throw_error(
      "create_directory", value, attributes, error);
  return result;
}

bool create_directories(const path& value, error_code& error) {
  if (value.empty()) {
    error.assign(EINVAL, generic_category());
    return false;
  }
  if (exists(value, error)) {
    if (error) return false;
    if (!is_directory(value, error) && !error)
      error.assign(ENOTDIR, generic_category());
    return false;
  }
  if (error) return false;
  path parent = value.parent_path();
  if (!parent.empty() && parent != value && !exists(parent, error)) {
    if (error || !create_directories(parent, error)) {
      if (error) return false;
    }
  }
  return create_directory(value, error);
}
bool create_directories(const path& value) {
  error_code error;
  bool result = create_directories(value, error);
  if (error)
    __filesystem_detail::__throw_error("create_directories", value, error);
  return result;
}

static void __create_link(const path& target, const path& link, int operation,
                          error_code& error) {
  __filesystem_detail::__error(
      error, __filesystem_detail::__call2(operation, target.c_str(),
                                          link.c_str()));
}
void create_symlink(const path& target, const path& link,
                    error_code& error) noexcept {
  __create_link(target, link, SYS_FS_CREATE_SYMLINK, error);
}
void create_symlink(const path& target, const path& link) {
  error_code error;
  create_symlink(target, link, error);
  if (error)
    __filesystem_detail::__throw_error("create_symlink", target, link, error);
}
void create_directory_symlink(const path& target, const path& link,
                              error_code& error) noexcept {
  create_symlink(target, link, error);
}
void create_directory_symlink(const path& target, const path& link) {
  create_symlink(target, link);
}
void create_hard_link(const path& target, const path& link,
                      error_code& error) noexcept {
  __create_link(target, link, SYS_FS_CREATE_HARD_LINK, error);
}
void create_hard_link(const path& target, const path& link) {
  error_code error;
  create_hard_link(target, link, error);
  if (error)
    __filesystem_detail::__throw_error("create_hard_link", target, link, error);
}

path read_symlink(const path& value, error_code& error) {
  size_t capacity = 256;
  for (;;) {
    vector<char> buffer(capacity);
    int64_t result =
        syscall(SYS_FS_READ_SYMLINK, value.c_str(), buffer.data(), capacity);
    if (result == -ENAMETOOLONG) {
      capacity *= 2;
      continue;
    }
    if (__filesystem_detail::__error(error, result)) return path();
    return path(buffer.data());
  }
}
path read_symlink(const path& value) {
  error_code error;
  path result = read_symlink(value, error);
  if (error)
    __filesystem_detail::__throw_error("read_symlink", value, error);
  return result;
}

void copy_symlink(const path& source, const path& destination,
                  error_code& error) noexcept {
  path target = read_symlink(source, error);
  if (!error) create_symlink(target, destination, error);
}
void copy_symlink(const path& source, const path& destination) {
  error_code error;
  copy_symlink(source, destination, error);
  if (error)
    __filesystem_detail::__throw_error("copy_symlink", source, destination,
                                        error);
}

bool copy_file(const path& source, const path& destination,
               copy_options options, error_code& error) {
  int mode = 0;
  if ((options & copy_options::skip_existing) != copy_options::none) mode |= 1;
  if ((options & copy_options::overwrite_existing) != copy_options::none)
    mode |= 2;
  if ((options & copy_options::update_existing) != copy_options::none) mode |= 4;
  int64_t result =
      syscall(SYS_FS_COPY_FILE, source.c_str(), destination.c_str(), mode);
  if (__filesystem_detail::__error(error, result)) return false;
  return result == 0;
}
bool copy_file(const path& source, const path& destination,
               error_code& error) {
  return copy_file(source, destination, copy_options::none, error);
}
bool copy_file(const path& source, const path& destination,
               copy_options options) {
  error_code error;
  bool result = copy_file(source, destination, options, error);
  if (error)
    __filesystem_detail::__throw_error("copy_file", source, destination, error);
  return result;
}
bool copy_file(const path& source, const path& destination) {
  return copy_file(source, destination, copy_options::none);
}

void copy(const path& source, const path& destination, copy_options options,
          error_code& error) {
  file_status source_status =
      (options & copy_options::copy_symlinks) != copy_options::none
          ? symlink_status(source, error)
          : status(source, error);
  if (error) return;
  if (is_symlink(source_status)) {
    if ((options & copy_options::skip_symlinks) != copy_options::none) {
      error.clear();
      return;
    }
    if ((options & copy_options::copy_symlinks) != copy_options::none) {
      copy_symlink(source, destination, error);
      return;
    }
  }
  if (is_regular_file(source_status)) {
    if ((options & copy_options::directories_only) != copy_options::none) {
      error.clear();
      return;
    }
    if ((options & copy_options::create_symlinks) != copy_options::none) {
      create_symlink(absolute(source, error), destination, error);
    } else if ((options & copy_options::create_hard_links) !=
               copy_options::none) {
      create_hard_link(source, destination, error);
    } else {
      copy_file(source, destination, options, error);
    }
    return;
  }
  if (!is_directory(source_status)) {
    error.assign(EINVAL, generic_category());
    return;
  }
  if ((options & copy_options::recursive) == copy_options::none &&
      (options & copy_options::directories_only) == copy_options::none) {
    error.assign(EISDIR, generic_category());
    return;
  }
  if (!exists(destination, error)) {
    if (error || !create_directory(destination, source, error)) return;
  }
  directory_iterator iterator(source, error);
  directory_iterator finish;
  while (!error && iterator != finish) {
    copy(iterator->path(), destination / iterator->path().filename(), options,
         error);
    if (!error) iterator.increment(error);
  }
}
void copy(const path& source, const path& destination, error_code& error) {
  copy(source, destination, copy_options::none, error);
}
void copy(const path& source, const path& destination, copy_options options) {
  error_code error;
  copy(source, destination, options, error);
  if (error)
    __filesystem_detail::__throw_error("copy", source, destination, error);
}
void copy(const path& source, const path& destination) {
  copy(source, destination, copy_options::none);
}

bool remove(const path& value, error_code& error) noexcept {
  int64_t result = syscall(SYS_FS_REMOVE, value.c_str());
  if (result == -ENOENT) {
    error.clear();
    return false;
  }
  return !__filesystem_detail::__error(error, result);
}
bool remove(const path& value) {
  error_code error;
  bool result = remove(value, error);
  if (error) __filesystem_detail::__throw_error("remove", value, error);
  return result;
}

uintmax_t remove_all(const path& value, error_code& error) {
  file_status value_status = symlink_status(value, error);
  if (error) return static_cast<uintmax_t>(-1);
  if (!exists(value_status)) {
    error.clear();
    return 0;
  }
  uintmax_t count = 0;
  if (is_directory(value_status)) {
    directory_iterator iterator(value, error);
    directory_iterator finish;
    while (!error && iterator != finish) {
      uintmax_t removed = remove_all(iterator->path(), error);
      if (error) return static_cast<uintmax_t>(-1);
      count += removed;
      iterator.increment(error);
    }
    if (error) return static_cast<uintmax_t>(-1);
  }
  if (!remove(value, error)) {
    return error ? static_cast<uintmax_t>(-1) : count;
  }
  return count + 1;
}
uintmax_t remove_all(const path& value) {
  error_code error;
  uintmax_t result = remove_all(value, error);
  if (error) __filesystem_detail::__throw_error("remove_all", value, error);
  return result;
}

void rename(const path& old_value, const path& new_value,
            error_code& error) noexcept {
  __filesystem_detail::__error(
      error, __filesystem_detail::__call2(SYS_FS_RENAME, old_value.c_str(),
                                          new_value.c_str()));
}
void rename(const path& old_value, const path& new_value) {
  error_code error;
  rename(old_value, new_value, error);
  if (error)
    __filesystem_detail::__throw_error("rename", old_value, new_value, error);
}

void resize_file(const path& value, uintmax_t size,
                 error_code& error) noexcept {
  uint64_t wire_size = static_cast<uint64_t>(size);
  __filesystem_detail::__error(
      error, __filesystem_detail::__call2(SYS_FS_RESIZE, value.c_str(),
                                          &wire_size));
}
void resize_file(const path& value, uintmax_t size) {
  error_code error;
  resize_file(value, size, error);
  if (error) __filesystem_detail::__throw_error("resize_file", value, error);
}

void permissions(const path& value, perms permissions_value,
                 perm_options options, error_code& error) noexcept {
  unsigned requested = static_cast<unsigned>(permissions_value);
  unsigned mode = requested & 07777;
  if ((options & (perm_options::add | perm_options::remove)) !=
      static_cast<perm_options>(0)) {
    file_status current =
        (options & perm_options::nofollow) != static_cast<perm_options>(0)
            ? symlink_status(value, error)
            : status(value, error);
    if (error) return;
    unsigned existing = static_cast<unsigned>(current.permissions()) & 07777;
    if ((options & perm_options::add) != static_cast<perm_options>(0))
      mode = existing | mode;
    else
      mode = existing & ~mode;
  }
  bool follow =
      (options & perm_options::nofollow) == static_cast<perm_options>(0);
  int64_t result =
      syscall(SYS_FS_SET_PERMISSIONS, value.c_str(), mode, follow ? 1 : 0);
  __filesystem_detail::__error(error, result);
}
void permissions(const path& value, perms permissions_value,
                 perm_options options) {
  error_code error;
  permissions(value, permissions_value, options, error);
  if (error) __filesystem_detail::__throw_error("permissions", value, error);
}

space_info space(const path& value, error_code& error) noexcept {
  __filesystem_detail::__wire_space wire;
  int64_t result = syscall(SYS_FS_SPACE, value.c_str(), &wire);
  if (__filesystem_detail::__error(error, result)) {
    uintmax_t invalid = static_cast<uintmax_t>(-1);
    return {invalid, invalid, invalid};
  }
  return {static_cast<uintmax_t>(wire.capacity),
          static_cast<uintmax_t>(wire.free),
          static_cast<uintmax_t>(wire.available)};
}
space_info space(const path& value) {
  error_code error;
  space_info result = space(value, error);
  if (error) __filesystem_detail::__throw_error("space", value, error);
  return result;
}

bool equivalent(const path& first, const path& second,
                error_code& error) noexcept {
  __filesystem_detail::__wire_status left;
  __filesystem_detail::__wire_status right;
  __filesystem_detail::__status(first, true, error, &left);
  if (error) return false;
  __filesystem_detail::__status(second, true, error, &right);
  return !error && left.device == right.device && left.inode == right.inode;
}
bool equivalent(const path& first, const path& second) {
  error_code error;
  bool result = equivalent(first, second, error);
  if (error)
    __filesystem_detail::__throw_error("equivalent", first, second, error);
  return result;
}

bool is_empty(const path& value, error_code& error) {
  if (is_directory(value, error)) {
    if (error) return false;
    return directory_iterator(value, error) == directory_iterator();
  }
  if (error) return false;
  return file_size(value, error) == 0 && !error;
}
bool is_empty(const path& value) {
  error_code error;
  bool result = is_empty(value, error);
  if (error) __filesystem_detail::__throw_error("is_empty", value, error);
  return result;
}

path temp_directory_path(error_code& error) {
  const char* names[] = {"TMPDIR", "TMP", "TEMP"};
  for (const char* name : names) {
    const char* value = getenv(name);
    if (value != nullptr && value[0] != '\0') {
      path result(value);
      if (is_directory(result, error)) return result;
      if (error) return path();
    }
  }
  path result("/tmp");
  if (!is_directory(result, error) && !error)
    error.assign(ENOTDIR, generic_category());
  return error ? path() : result;
}
path temp_directory_path() {
  error_code error;
  path result = temp_directory_path(error);
  if (error)
    __filesystem_detail::__throw_error("temp_directory_path", path(), error);
  return result;
}

void directory_entry::refresh(error_code& error) noexcept {
  filesystem::status(__path, error);
}
void directory_entry::refresh() {
  error_code error;
  refresh(error);
  if (error)
    __filesystem_detail::__throw_error("directory_entry::refresh", __path,
                                        error);
}

#define DAVECC_DIRECTORY_ENTRY_PREDICATE(name)                         \
  bool directory_entry::name(error_code& error) const noexcept {       \
    return filesystem::name(__path, error);                             \
  }                                                                     \
  bool directory_entry::name() const { return filesystem::name(__path); }

DAVECC_DIRECTORY_ENTRY_PREDICATE(exists)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_block_file)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_character_file)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_directory)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_fifo)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_other)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_regular_file)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_socket)
DAVECC_DIRECTORY_ENTRY_PREDICATE(is_symlink)

#undef DAVECC_DIRECTORY_ENTRY_PREDICATE

uintmax_t directory_entry::file_size(error_code& error) const noexcept {
  return filesystem::file_size(__path, error);
}
uintmax_t directory_entry::file_size() const {
  return filesystem::file_size(__path);
}
uintmax_t directory_entry::hard_link_count(error_code& error) const noexcept {
  return filesystem::hard_link_count(__path, error);
}
uintmax_t directory_entry::hard_link_count() const {
  return filesystem::hard_link_count(__path);
}
file_time_type directory_entry::last_write_time(
    error_code& error) const noexcept {
  return filesystem::last_write_time(__path, error);
}
file_time_type directory_entry::last_write_time() const {
  return filesystem::last_write_time(__path);
}
file_status directory_entry::status(error_code& error) const noexcept {
  return filesystem::status(__path, error);
}
file_status directory_entry::status() const {
  return filesystem::status(__path);
}
file_status directory_entry::symlink_status(error_code& error) const noexcept {
  return filesystem::symlink_status(__path, error);
}
file_status directory_entry::symlink_status() const {
  return filesystem::symlink_status(__path);
}

}  // namespace filesystem
}  // namespace std
