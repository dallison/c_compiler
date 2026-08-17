#include <__exception_support>
#include <__hive_storage.h>
#include <stdexcept>
#include <utility>

namespace std {
namespace __hive_detail {

static constexpr size_t __npos = static_cast<size_t>(-1);

void __throw_length_error() {
  __DAVECC_THROW(length_error("hive capacity exceeds max_size"));
}

void __storage::__link_active_back(__block_base* block) noexcept {
  block->__prev = __end_.__prev;
  block->__next = &__end_;
  __end_.__prev->__next = block;
  __end_.__prev = block;
}

void __storage::__unlink_active(__block_base* block) noexcept {
  block->__prev->__next = block->__next;
  block->__next->__prev = block->__prev;
  block->__prev = nullptr;
  block->__next = nullptr;
}

void __storage::__add_hole_block(__block_base* block) noexcept {
  block->__hole_prev = nullptr;
  block->__hole_next = __holes_;
  if (__holes_ != nullptr) {
    __holes_->__hole_prev = block;
  }
  __holes_ = block;
}

void __storage::__remove_hole_block(__block_base* block) noexcept {
  if (__holes_ == block) {
    __holes_ = block->__hole_next;
  } else if (block->__hole_prev != nullptr) {
    block->__hole_prev->__hole_next = block->__hole_next;
  }
  if (block->__hole_next != nullptr) {
    block->__hole_next->__hole_prev = block->__hole_prev;
  }
  block->__hole_prev = nullptr;
  block->__hole_next = nullptr;
}

void __storage::__add_run(__block_base* block, size_t start) noexcept {
  if (block->__free_run == __npos) {
    __add_hole_block(block);
  }
  block->__run_prev[start] = __npos;
  block->__run_next[start] = block->__free_run;
  if (block->__free_run != __npos) {
    block->__run_prev[block->__free_run] = start;
  }
  block->__free_run = start;
}

void __storage::__remove_run(__block_base* block, size_t start) noexcept {
  size_t previous = block->__run_prev[start];
  size_t next = block->__run_next[start];
  if (previous == __npos) {
    block->__free_run = next;
  } else {
    block->__run_next[previous] = next;
  }
  if (next != __npos) {
    block->__run_prev[next] = previous;
  }
  if (block->__free_run == __npos) {
    __remove_hole_block(block);
  }
}

void __storage::__replace_run(__block_base* block, size_t old_start,
                              size_t new_start) noexcept {
  size_t previous = block->__run_prev[old_start];
  size_t next = block->__run_next[old_start];
  block->__run_prev[new_start] = previous;
  block->__run_next[new_start] = next;
  if (previous == __npos) {
    block->__free_run = new_start;
  } else {
    block->__run_next[previous] = new_start;
  }
  if (next != __npos) {
    block->__run_prev[next] = new_start;
  }
}

void __storage::__mark_erased(__block_base* block, size_t index) noexcept {
  size_t left =
      index != 0 && block->__skip[index - 1] != 0
          ? block->__skip[index - 1]
          : 0;
  size_t right =
      index + 1 < block->__used && block->__skip[index + 1] != 0
          ? block->__skip[index + 1]
          : 0;
  size_t start = index - left;
  size_t finish = index + right;
  size_t length = left + 1 + right;

  if (left == 0 && right == 0) {
    __add_run(block, index);
  } else if (left == 0) {
    __replace_run(block, index + 1, index);
  } else if (right != 0) {
    __remove_run(block, index + 1);
  }

  block->__skip[index] = 1;
  block->__skip[start] = length;
  block->__skip[finish] = length;
}

void __storage::__consume_run_start(__block_base* block,
                                    size_t start) noexcept {
  size_t length = block->__skip[start];
  if (length == 1) {
    __remove_run(block, start);
  } else {
    size_t new_start = start + 1;
    size_t finish = start + length - 1;
    __replace_run(block, start, new_start);
    block->__skip[new_start] = length - 1;
    block->__skip[finish] = length - 1;
  }
  block->__skip[start] = 0;
}

void __storage::__reset_as_reserved(__block_base* block) noexcept {
  block->__used = 0;
  block->__live = 0;
  block->__free_run = __npos;
  block->__first = 0;
  block->__last = 0;
  block->__hole_prev = nullptr;
  block->__hole_next = nullptr;
  block->__reserved_next = __reserved_;
  __reserved_ = block;
}

__block_base* __storage::__take_reserved() noexcept {
  __block_base* block = __reserved_;
  if (block != nullptr) {
    __reserved_ = block->__reserved_next;
    block->__reserved_next = nullptr;
  }
  return block;
}

void __storage::__renumber() noexcept {
  size_t order = 0;
  for (__block_base* block = __end_.__next; block != &__end_;
       block = block->__next) {
    block->__order = order++;
  }
  __end_.__order = order;
}

void __storage::__rebuild_holes() noexcept {
  __holes_ = nullptr;
  for (__block_base* block = __end_.__next; block != &__end_;
       block = block->__next) {
    block->__hole_prev = nullptr;
    block->__hole_next = nullptr;
    if (block->__free_run != __npos) {
      __add_hole_block(block);
    }
  }
}

void __storage::__steal_storage(__storage& other) noexcept {
  if (other.__end_.__next != &other.__end_) {
    __block_base* first = other.__end_.__next;
    __block_base* last = other.__end_.__prev;
    first->__prev = &__end_;
    last->__next = &__end_;
    __end_.__next = first;
    __end_.__prev = last;
  }
  __reserved_ = other.__reserved_;
  __holes_ = other.__holes_;
  __size_ = other.__size_;
  __capacity_ = other.__capacity_;
  other.__init_sentinel();
  __renumber();
}

void __storage::__swap_storage(__storage& other) noexcept {
  __block_base* this_first =
      __end_.__next == &__end_ ? nullptr : __end_.__next;
  __block_base* this_last =
      __end_.__prev == &__end_ ? nullptr : __end_.__prev;
  __block_base* other_first =
      other.__end_.__next == &other.__end_ ? nullptr : other.__end_.__next;
  __block_base* other_last =
      other.__end_.__prev == &other.__end_ ? nullptr : other.__end_.__prev;

  if (other_first == nullptr) {
    __end_.__next = &__end_;
    __end_.__prev = &__end_;
  } else {
    __end_.__next = other_first;
    __end_.__prev = other_last;
    other_first->__prev = &__end_;
    other_last->__next = &__end_;
  }
  if (this_first == nullptr) {
    other.__end_.__next = &other.__end_;
    other.__end_.__prev = &other.__end_;
  } else {
    other.__end_.__next = this_first;
    other.__end_.__prev = this_last;
    this_first->__prev = &other.__end_;
    this_last->__next = &other.__end_;
  }

  std::swap(__reserved_, other.__reserved_);
  std::swap(__holes_, other.__holes_);
  std::swap(__size_, other.__size_);
  std::swap(__capacity_, other.__capacity_);
  __renumber();
  other.__renumber();
}

void __storage::__splice_storage(__storage& other, size_t minimum,
                                 size_t maximum) {
  if (&other == this) {
    return;
  }
  for (__block_base* block = other.__end_.__next;
       block != &other.__end_; block = block->__next) {
    if (block->__capacity < minimum || block->__capacity > maximum) {
      __throw_length_error();
    }
  }
  if (other.__size_ == 0) {
    return;
  }

  __block_base* first = other.__end_.__next;
  __block_base* last = other.__end_.__prev;
  first->__prev = __end_.__prev;
  __end_.__prev->__next = first;
  last->__next = &__end_;
  __end_.__prev = last;

  other.__end_.__next = &other.__end_;
  other.__end_.__prev = &other.__end_;

  size_t transferred_capacity = 0;
  for (__block_base* block = first;; block = block->__next) {
    transferred_capacity += block->__capacity;
    if (block == last) {
      break;
    }
  }
  __size_ += other.__size_;
  other.__size_ = 0;
  __capacity_ += transferred_capacity;
  other.__capacity_ -= transferred_capacity;
  __rebuild_holes();
  other.__holes_ = nullptr;
  __renumber();
  other.__renumber();
}

}  // namespace __hive_detail
}  // namespace std
