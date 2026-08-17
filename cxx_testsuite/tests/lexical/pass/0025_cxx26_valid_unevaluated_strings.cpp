// RUN: -std=c++26

static_assert(true, "simple escape: \n; UCN: \u0393");
static_assert(true, R"(raw \x41 text)");

[[deprecated("use \u0394 instead")]]
void old_function();

extern "C" int c_function();

void removed() = delete("reason\n");
