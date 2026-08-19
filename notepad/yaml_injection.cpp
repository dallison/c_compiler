// Compile with:
//   davecc -std=c++29 -isystem libc/include notepad/yaml_injection.cpp
//
// This deliberately implements a small YAML subset: integer scalars, flow
// sequences of integers, and one-level nested integer mappings. Blank lines,
// indentation, and comments are accepted.

#include <cstdio>
#include <map>
#include <meta>
#include <string>
#include <vector>

namespace tiny_yaml {

constexpr bool horizontal_space(char value) {
  return value == ' ' || value == '\t' || value == '\r';
}

constexpr const char* scalar(const char* document, const char* wanted_key) {
  const char* line = document;
  while (*line != '\0') {
    while (horizontal_space(*line)) {
      ++line;
    }
    if (*line == '\n') {
      ++line;
      continue;
    }
    if (*line == '#') {
      while (*line != '\0' && *line != '\n') {
        ++line;
      }
      continue;
    }

    const char* cursor = line;
    const char* key = wanted_key;
    while (*key != '\0' && *cursor == *key) {
      ++cursor;
      ++key;
    }
    while (horizontal_space(*cursor)) {
      ++cursor;
    }

    if (*key == '\0' && *cursor == ':') {
      ++cursor;
      while (horizontal_space(*cursor)) {
        ++cursor;
      }
      return cursor;
    }

    while (*line != '\0' && *line != '\n') {
      ++line;
    }
  }
  throw "missing YAML key";
}

constexpr int parse_integer(const char*& cursor) {
  int sign = 1;
  if (*cursor == '-') {
    sign = -1;
    ++cursor;
  } else if (*cursor == '+') {
    ++cursor;
  }
  if (*cursor < '0' || *cursor > '9') {
    throw "expected a YAML integer";
  }

  int result = 0;
  while (*cursor >= '0' && *cursor <= '9') {
    result = result * 10 + (*cursor - '0');
    ++cursor;
  }
  return sign * result;
}

consteval int integer(const char* document, const char* wanted_key) {
  const char* cursor = scalar(document, wanted_key);
  int result = parse_integer(cursor);
  while (horizontal_space(*cursor)) {
    ++cursor;
  }
  if (*cursor != '\0' && *cursor != '\n' && *cursor != '#') {
    throw "unexpected character after YAML integer";
  }
  return result;
}

std::vector<int> integer_vector(const char* document, const char* wanted_key) {
  const char* cursor = scalar(document, wanted_key);
  if (*cursor != '[') {
    throw "expected a YAML flow sequence";
  }
  ++cursor;

  std::vector<int> result;
  for (;;) {
    while (horizontal_space(*cursor)) {
      ++cursor;
    }
    if (*cursor == ']') {
      return result;
    }
    result.push_back(parse_integer(cursor));
    while (horizontal_space(*cursor)) {
      ++cursor;
    }
    if (*cursor == ']') {
      return result;
    }
    if (*cursor != ',') {
      throw "expected ',' in YAML flow sequence";
    }
    ++cursor;
  }
}

std::map<std::string, int> integer_map(const char* document,
                                       const char* wanted_key) {
  const char* cursor = scalar(document, wanted_key);
  if (*cursor != '\n') {
    throw "expected a nested YAML mapping";
  }
  ++cursor;

  std::map<std::string, int> result;
  while (*cursor != '\0') {
    const char* line = cursor;
    while (horizontal_space(*cursor)) {
      ++cursor;
    }
    if (*cursor == '\n') {
      ++cursor;
      continue;
    }
    if (*cursor == '#') {
      while (*cursor != '\0' && *cursor != '\n') {
        ++cursor;
      }
      if (*cursor == '\n') {
        ++cursor;
      }
      continue;
    }
    if (cursor == line) {
      break;
    }

    const char* key_begin = cursor;
    while (*cursor != '\0' && *cursor != '\n' && *cursor != ':') {
      ++cursor;
    }
    const char* key_end = cursor;
    while (key_end != key_begin && horizontal_space(key_end[-1])) {
      --key_end;
    }
    if (*cursor != ':') {
      throw "expected ':' in nested YAML mapping";
    }
    ++cursor;
    while (horizontal_space(*cursor)) {
      ++cursor;
    }

    std::string key(key_begin, static_cast<size_t>(key_end - key_begin));
    result[key] = parse_integer(cursor);
    while (*cursor != '\0' && *cursor != '\n') {
      ++cursor;
    }
    if (*cursor == '\n') {
      ++cursor;
    }
  }
  return result;
}

}  // namespace tiny_yaml

constexpr char yaml_definition[] =
    "# One complete YAML document\n"
    "port: 8080\n"
    "workers: 4\n"
    "retry_delays: [100, 250, 500]\n"
    "service_ports:\n"
    "  http: 8080\n"
    "  admin: 9090\n";

constexpr int parsed_port = tiny_yaml::integer(yaml_definition, "port");
constexpr int parsed_workers = tiny_yaml::integer(yaml_definition, "workers");

struct server_config {
  consteval {
    std::meta::queue_injection(
        ^{
          int \id("server_", "port");
          int \id("server_", "workers");
          std::vector<int> retry_delays;
          std::map<std::string, int> service_ports;
        });
  }
};

static_assert(parsed_port == 8080);
static_assert(parsed_workers == 4);

void print(const server_config& value) {
  std::printf("server_config {\n");
  std::printf("  server_port: %d,\n", value.server_port);
  std::printf("  server_workers: %d,\n", value.server_workers);

  std::printf("  retry_delays: [");
  for (size_t i = 0; i < value.retry_delays.size(); ++i) {
    std::printf("%s%d", i == 0 ? "" : ", ", value.retry_delays[i]);
  }
  std::printf("],\n");

  std::printf("  service_ports: {\n");
  for (auto entry = value.service_ports.begin();
       entry != value.service_ports.end(); ++entry) {
    std::printf("    %s: %d\n", entry->first.c_str(), entry->second);
  }
  std::printf("  }\n");
  std::printf("}\n");
}

int main() {
  server_config configuration{
      parsed_port,
      parsed_workers,
      tiny_yaml::integer_vector(yaml_definition, "retry_delays"),
      tiny_yaml::integer_map(yaml_definition, "service_ports"),
  };
  print(configuration);
  return configuration.server_port == 8080 &&
                 configuration.server_workers == 4 &&
                 configuration.retry_delays.size() == 3 &&
                 configuration.retry_delays[0] == 100 &&
                 configuration.retry_delays[2] == 500 &&
                 configuration.service_ports["http"] == 8080 &&
                 configuration.service_ports["admin"] == 9090
             ? 0
             : 1;
}
