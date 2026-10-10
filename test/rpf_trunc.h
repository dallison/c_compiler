// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// RepeatedField and RepeatedPtrField are used by generated protocol message
// classes to manipulate repeated fields.  These classes are very similar to
// STL's vector, but include a number of optimizations found to be useful
// specifically in the case of Protocol Buffers.  RepeatedPtrField is
// particularly different from STL vector as it manages ownership of the
// pointers that it contains.
//
// This header covers RepeatedPtrField.

#ifndef GOOGLE_PROTOBUF_REPEATED_PTR_FIELD_H__
#define GOOGLE_PROTOBUF_REPEATED_PTR_FIELD_H__

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"
#include "absl/functional/function_ref.h"
#include "absl/log/absl_check.h"
#include "absl/meta/type_traits.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/internal_visibility.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#ifdef SWIG
#error "You cannot SWIG proto headers"
#endif

namespace google {
namespace protobuf {

class Message;
class Reflection;
class DynamicMessage;

template <typename T>
struct WeakRepeatedPtrField;

namespace internal {

class MergePartialFromCodedStreamHelper;
class SwapFieldHelper;

class MapFieldBase;

template <typename It>
class RepeatedPtrIterator;
template <typename It, typename VoidPtr>
class RepeatedPtrOverPtrsIterator;
template <typename T>
class AllocatedRepeatedPtrFieldBackInsertIterator;

// Swaps two non-overlapping blocks of memory of size `N`
template <size_t N>
inline void memswap(char* PROTOBUF_RESTRICT a, char* PROTOBUF_RESTRICT b) {
  // `PROTOBUF_RESTRICT` tells compiler that blocks do not overlapping which
  // allows it to generate optimized code for swap_ranges.
  std::swap_ranges(a, a + N, b);
}

// A trait that tells offset of `T::arena_`.
//
// Do not use this struct - it exists for internal use only.
template <typename T>
struct ArenaOffsetHelper {
  static constexpr size_t value = offsetof(T, arena_);
};

// Copies the object in the arena.
// Used in the slow path. Out-of-line for lower binary size cost.
PROTOBUF_EXPORT MessageLite* CloneSlow(Arena* arena, const MessageLite& value);
PROTOBUF_EXPORT std::string* CloneSlow(Arena* arena, const std::string& value);

// A utility function for logging that doesn't need any template types.
PROTOBUF_EXPORT void LogIndexOutOfBounds(int index, int size);

// A utility function for logging that doesn't need any template types. Same as
// LogIndexOutOfBounds, but aborts the program in all cases by logging to FATAL
// instead of DFATAL.
// TODO: Remove preserve_all and add no_return once experiment is
// complete.
PROTOBUF_PRESERVE_ALL PROTOBUF_EXPORT void LogIndexOutOfBoundsAndAbort(
    int index, int size);
PROTOBUF_EXPORT inline void RuntimeAssertInBounds(int index, int size) {
  if constexpr (GetBoundsCheckMode() == BoundsCheckMode::kAbort) {
    if (ABSL_PREDICT_FALSE(index < 0 || index >= size)) {
      LogIndexOutOfBoundsAndAbort(index, size);
    }
  }
  ABSL_DCHECK_GE(index, 0);
  ABSL_DCHECK_LT(index, size);
}

// Defined further below.
template <typename Type>
class GenericTypeHandler;

using ElementNewFn = void(Arena*, void*& ptr);

// This is the common base class for RepeatedPtrFields.  It deals only in void*
// pointers.  Users should not use this interface directly.
//
// The methods of this interface correspond to the methods of RepeatedPtrField,
// but may have a template argument called TypeHandler.  Its signature is:
//   class TypeHandler {
//    public:
//     using Type = MyType;
//
//     static Type*(*)(Arena*) GetNewFunc();
//     static Type*(*)(Arena*) GetNewFromPrototypeFunc(const Type* prototype);
//     static Arena* GetArena(Type* value);
//
//     static Type* New(Arena* arena, Type&& value);
//     static void Delete(Type*, Arena* arena);
//     static void Clear(Type*);
//
//     // Only needs to be implemented if SpaceUsedExcludingSelf() is called.
//     static int SpaceUsedLong(const Type&);
//
//     static const Type& default_instance();
//   };
class PROTOBUF_EXPORT RepeatedPtrFieldBase {
  template <typename TypeHandler>
  using Value = typename TypeHandler::Type;

  static constexpr int kSSOCapacity = 1;

 protected:
  // We use the same TypeHandler for all Message types to deduplicate generated
  // code.
  template <typename TypeHandler>
  using CommonHandler = typename std::conditional<
      std::is_base_of<MessageLite, Value<TypeHandler>>::value,
      GenericTypeHandler<MessageLite>, TypeHandler>::type;

  constexpr RepeatedPtrFieldBase()
      : tagged_rep_or_elem_(nullptr), current_size_(0), arena_(nullptr) {}
  explicit RepeatedPtrFieldBase(Arena* arena)
      : tagged_rep_or_elem_(nullptr), current_size_(0), arena_(arena) {}

  RepeatedPtrFieldBase(const RepeatedPtrFieldBase&) = delete;
  RepeatedPtrFieldBase& operator=(const RepeatedPtrFieldBase&) = delete;

  ~RepeatedPtrFieldBase() {
#ifndef NDEBUG
    // Try to trigger segfault / asan failure in non-opt builds if the arena
    // lifetime has ended before the destructor. Note that `GetArena()` is
    // not free, but this is debug-only.
    const Arena* arena = GetArena();
    if (arena != nullptr) (void)arena->SpaceAllocated();
#endif
  }

  bool empty() const { return current_size_ == 0; }
  int size() const { return current_size_; }
  // Returns the size of the buffer with pointers to elements.
  //
  // Note:
  //
  //   * prefer `SizeAtCapacity()` to `size() == Capacity()`;
  //   * prefer `AllocatedSizeAtCapacity()` to `allocated_size() == Capacity()`.
  int Capacity() const { return using_sso() ? kSSOCapacity : rep()->capacity; }

  template <typename TypeHandler>
  const Value<TypeHandler>& at(int index) const {
    ABSL_CHECK_GE(index, 0);
    ABSL_CHECK_LT(index, current_size_);
    return *cast<TypeHandler>(element_at(index));
  }

  template <typename TypeHandler>
  Value<TypeHandler>& at(int index) {
    ABSL_CHECK_GE(index, 0);
    ABSL_CHECK_LT(index, current_size_);
    return *cast<TypeHandler>(element_at(index));
  }

  template <typename TypeHandler>
  Value<TypeHandler>* Mutable(int index) {
      RuntimeAssertInBounds(index, current_size_);
    return cast<TypeHandler>(element_at(index));
  }

  template <typename TypeHandler>
  Value<TypeHandler>* Add(Arena* arena) {
    return cast<TypeHandler>(AddInternal(arena, TypeHandler::GetNewFunc()));
  }

  template <typename TypeHandler>
  void Add(Arena* arena, Value<TypeHandler>&& value) {
    if (ClearedCount() > 0) {
      *cast<TypeHandler>(element_at(ExchangeCurrentSize(current_size_ + 1))) =
          std::move(value);
    } else {
      AddInternal(arena, TypeHandler::GetNewWithMoveFunc(std::move(value)));
    }
  }

  // Must be called from destructor.
  //
  // Pre-condition: NeedsDestroy() returns true.
  template <typename TypeHandler>
  void Destroy() {
    ABSL_DCHECK(NeedsDestroy());

    // TODO: arena check is redundant once all `RepeatedPtrField`s
    // with non-null arena are owned by the arena.
    if (ABSL_PREDICT_FALSE(GetArena() != nullptr)) return;

    using H = CommonHandler<TypeHandler>;
    int n = allocated_size();
    ABSL_DCHECK_LE(n, Capacity());
    void** elems = elements();
    for (int i = 0; i < n; i++) {
      if (i + 5 < n) {
        absl::PrefetchToLocalCacheNta(elems[i + 5]);
      }
      Delete<H>(elems[i], nullptr);
    }
    if (!using_sso()) {
      internal::SizedDelete(rep(),
                            Capacity() * sizeof(elems[0]) + kRepHeaderSize);
    }
  }

  inline bool NeedsDestroy() const {
    // Either there is an allocated element in SSO buffer or there is an
    // allocated Rep.
    return tagged_rep_or_elem_ != nullptr;
  }

  // Pre-condition: NeedsDestroy() returns true.
  void DestroyProtos();

 public:
  // The next few methods are public so that they can be called from generated
  // code when implicit weak fields are used, but they should never be called by
  // application code.

  template <typename TypeHandler>
  PROTOBUF_FUTURE_ADD_NODISCARD const Value<TypeHandler>& Get(int index) const {
    if constexpr (GetBoundsCheckMode() == BoundsCheckMode::kReturnDefault) {
      if (ABSL_PREDICT_FALSE(index < 0 || index >= current_size_)) {
        // `default_instance()` is not supported for MessageLite and Message.
        if constexpr (TypeHandler::has_default_instance()) {
          LogIndexOutOfBounds(index, current_size_);
          return TypeHandler::default_instance();
        }
      }
    }
    // We refactor this to a separate function instead of inlining it so we
    // can measure the performance impact more easily.
    RuntimeAssertInBounds(index, current_size_);
    return *cast<TypeHandler>(element_at(index));
  }

  // Creates and adds an element using the given prototype, without introducing
  // a link-time dependency on the concrete message type.
  //
  // Pre-condition: `prototype` must not be nullptr.
  template <typename TypeHandler>
  PROTOBUF_ALWAYS_INLINE Value<TypeHandler>* AddFromPrototype(
      Arena* arena, const Value<TypeHandler>* prototype) {
    using H = CommonHandler<TypeHandler>;
    Value<TypeHandler>* result = cast<TypeHandler>(
        AddInternal(arena, H::GetNewFromPrototypeFunc(prototype)));
    return result;
  }

  // Creates and adds an element using the given ClassData, without introducing
  // a link-time dependency on the concrete message type. This is generally
  // faster and should be preferred over the equivalent `AddFromPrototype()` if
  // the caller already has or can get the `ClassData`, and especially if
  // elements are added repeatedly.
  //
  // Pre-condition: `class_data` must not be nullptr.
  template <typename TypeHandler>
  PROTOBUF_ALWAYS_INLINE Value<TypeHandler>* AddFromClassData(
      Arena* arena, const ClassData* class_data) {
    using H = CommonHandler<TypeHandler>;
    Value<TypeHandler>* result = cast<TypeHandler>(
        AddInternal(arena, H::GetNewFromClassDataFunc(class_data)));
    return result;
  }

  template <typename TypeHandler>
  void Clear() {
    const int n = current_size_;
    ABSL_DCHECK_GE(n, 0);
    if (n > 0) {
      using H = CommonHandler<TypeHandler>;
      ClearNonEmpty<H>();
    }
  }

  // Appends all message values from `from` to this instance.
  template <typename T>
  void MergeFrom(const RepeatedPtrFieldBase& from, Arena* arena) {
    static_assert(std::is_base_of<MessageLite, T>::value, "");
    if constexpr (!std::is_base_of<Message, T>::value) {
      // For LITE objects we use the generic MergeFrom to save on binary size.
      return MergeFrom<MessageLite>(from, arena);
    }
    MergeFromConcreteMessage(from, arena, Arena::CopyConstruct<T>);
  }

  inline void InternalSwap(RepeatedPtrFieldBase* PROTOBUF_RESTRICT rhs) {
    ABSL_DCHECK(this != rhs);

    // Swap all fields except arena pointer at once.
    internal::memswap<ArenaOffsetHelper<RepeatedPtrFieldBase>::value>(
        reinterpret_cast<char*>(this), reinterpret_cast<char*>(rhs));
  }

  // Returns true if there are no preallocated elements in the array.
  PROTOBUF_FUTURE_ADD_NODISCARD bool PrepareForParse() {
    return allocated_size() == current_size_;
  }

  // Similar to `AddAllocated` but faster.
  //
  // Pre-condition: PrepareForParse() is true.
  void AddAllocatedForParse(void* value, Arena* arena) {
    ABSL_DCHECK(PrepareForParse());
    if (ABSL_PREDICT_FALSE(SizeAtCapacity())) {
      *InternalExtend(1, arena) = value;
      ++rep()->allocated_size;
    } else {
      if (using_sso()) {
        tagged_rep_or_elem_ = value;
      } else {
        rep()->elements[current_size_] = value;
        ++rep()->allocated_size;
      }
    }
    ExchangeCurrentSize(current_size_ + 1);
  }

 protected:
  template <typename TypeHandler>
  void RemoveLast() {
    ABSL_DCHECK_GT(current_size_, 0);
    ExchangeCurrentSize(current_size_ - 1);
    using H = CommonHandler<TypeHandler>;
    H::Clear(cast<H>(element_at(current_size_)));
  }

  template <typename TypeHandler>
  void CopyFrom(const RepeatedPtrFieldBase& other, Arena* arena) {
    ABSL_DCHECK_EQ(arena, GetArena());
    if (&other == this) return;
    Clear<TypeHandler>();
    if (other.empty()) return;
    MergeFrom<typename TypeHandler::Type>(other, arena);
  }

  void CloseGap(int start, int num);

  void Reserve(int capacity, Arena* arena);

  template <typename TypeHandler>
  static inline Value<TypeHandler>* copy(const Value<TypeHandler>* value) {
    return cast<TypeHandler>(CloneSlow(nullptr, *value));
  }

  // Used for constructing iterators.
  void* const* raw_data() const { return elements(); }
  void** raw_mutable_data() { return elements(); }

  template <typename TypeHandler>
  Value<TypeHandler>** mutable_data() {
    // TODO:  Breaks C++ aliasing rules.  We should probably remove this
    //   method entirely.
    return reinterpret_cast<Value<TypeHandler>**>(raw_mutable_data());
  }

#endif // bisect
