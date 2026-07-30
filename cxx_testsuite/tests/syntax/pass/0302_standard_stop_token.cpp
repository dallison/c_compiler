// RUN: -target x86_64 -std=c++20

#include <stop_token>
#include <type_traits>

static_assert(__cpp_lib_jthread == 201911L);
static_assert(std::is_default_constructible_v<std::stop_token>);
static_assert(std::is_copy_constructible_v<std::stop_token>);
static_assert(std::is_copy_constructible_v<std::stop_source>);
static_assert(!std::is_copy_constructible_v<
              std::stop_callback<void (*)()>>);

void callback() {}

void check_stop_interfaces() {
  std::stop_source source;
  std::stop_token token = source.get_token();
  std::stop_callback registered(token, &callback);
  (void)source.stop_possible();
  (void)source.stop_requested();
  (void)source.request_stop();

  std::stop_source disabled(std::nostopstate);
  (void)disabled.get_token();
}
