#include <list>

namespace std {
namespace __list_detail {

void __transfer(__node_base* pos, __node_base* first, __node_base* last) {
  if (pos == first || first == last) {
    return;
  }
  __node_base* last_prev = last->__prev;
  first->__prev->__next = last;
  last->__prev = first->__prev;
  __node_base* pos_prev = pos->__prev;
  pos_prev->__next = first;
  first->__prev = pos_prev;
  last_prev->__next = pos;
  pos->__prev = last_prev;
}

}  // namespace __list_detail
}  // namespace std
