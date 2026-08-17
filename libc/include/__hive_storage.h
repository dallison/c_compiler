#ifndef __davecc___hive_storage_h__
#define __davecc___hive_storage_h__

#include <stddef.h>

namespace std {
namespace __hive_detail {

struct __block_base {
  __block_base* __prev;
  __block_base* __next;
  __block_base* __hole_prev;
  __block_base* __hole_next;
  __block_base* __reserved_next;
  void* __elements;
  size_t* __skip;
  size_t* __run_prev;
  size_t* __run_next;
  size_t __capacity;
  size_t __used;
  size_t __live;
  size_t __free_run;
  size_t __first;
  size_t __last;
  size_t __order;
  bool __sentinel;

  constexpr __block_base() noexcept
      : __prev(nullptr),
        __next(nullptr),
        __hole_prev(nullptr),
        __hole_next(nullptr),
        __reserved_next(nullptr),
        __elements(nullptr),
        __skip(nullptr),
        __run_prev(nullptr),
        __run_next(nullptr),
        __capacity(0),
        __used(0),
        __live(0),
        __free_run(static_cast<size_t>(-1)),
        __first(0),
        __last(0),
        __order(0),
        __sentinel(false) {}
};

class __storage {
 public:
  __block_base __end_;
  __block_base* __reserved_;
  __block_base* __holes_;
  size_t __size_;
  size_t __capacity_;

  constexpr __storage() noexcept
      : __end_(),
        __reserved_(nullptr),
        __holes_(nullptr),
        __size_(0),
        __capacity_(0) {
    __init_sentinel();
  }

  constexpr void __init_sentinel() noexcept {
    __end_.__prev = &__end_;
    __end_.__next = &__end_;
    __end_.__sentinel = true;
    __end_.__order = 0;
    __reserved_ = nullptr;
    __holes_ = nullptr;
    __size_ = 0;
    __capacity_ = 0;
  }

  void __link_active_back(__block_base* block) noexcept;
  void __unlink_active(__block_base* block) noexcept;
  void __add_hole_block(__block_base* block) noexcept;
  void __remove_hole_block(__block_base* block) noexcept;
  void __add_run(__block_base* block, size_t start) noexcept;
  void __remove_run(__block_base* block, size_t start) noexcept;
  void __replace_run(__block_base* block, size_t old_start,
                     size_t new_start) noexcept;
  void __mark_erased(__block_base* block, size_t index) noexcept;
  void __consume_run_start(__block_base* block, size_t start) noexcept;
  void __reset_as_reserved(__block_base* block) noexcept;
  __block_base* __take_reserved() noexcept;
  void __renumber() noexcept;
  void __rebuild_holes() noexcept;
  void __steal_storage(__storage& other) noexcept;
  void __swap_storage(__storage& other) noexcept;
  void __splice_storage(__storage& other, size_t minimum,
                        size_t maximum);
};

void __throw_length_error();

}  // namespace __hive_detail
}  // namespace std

#endif  // __davecc___hive_storage_h__
