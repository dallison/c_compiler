// RUN: -std=c++20

constexpr char document[] =
    "# one complete configuration document\nport: 8080\nworkers: 4\n";

static_assert(sizeof(document) == 61);
static_assert(document[38] == 'p');
static_assert(document[49] == 'w');
