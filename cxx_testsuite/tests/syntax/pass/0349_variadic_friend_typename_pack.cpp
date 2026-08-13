// RUN: -std=c++26

struct NestedFriend {
  struct Type {};
};

template <class... Owners>
struct NestedOwner {
  friend typename Owners::Type...;
};

template <class... Owners>
struct NestedOwnerWithoutTypename {
  friend Owners::Type...;
};

NestedOwner<NestedFriend> owner;
NestedOwnerWithoutTypename<NestedFriend> owner_without_typename;
