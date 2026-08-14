#include <contracts>

using std::contracts::assertion_kind;
using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::evaluation_semantic;

static void __davecc_default_contract_violation_handler(
    const contract_violation&) {
}

[[gnu::weak]] void handle_contract_violation(const contract_violation&);

void handle_contract_violation(const contract_violation& violation) {
  __davecc_default_contract_violation_handler(violation);
}

struct __davecc_contract_runtime_access {
  static void report(int kind, int semantic, int detection, int line,
                     int column, const char* comment, const char* file,
                     const char* function) {
    contract_violation violation(
        comment, (detection_mode)detection, semantic != 2,
        (assertion_kind)kind,
        std::source_location((unsigned int)line, (unsigned int)column,
                             file, function),
        (evaluation_semantic)semantic);
    handle_contract_violation(violation);
  }
};

extern "C" void __davecc_contract_violation(
    int kind, int semantic, int detection, int line, int column,
    const char* comment, const char* file, const char* function) {
  __davecc_contract_runtime_access::report(
      kind, semantic, detection, line, column, comment, file, function);
}

namespace std {
namespace contracts {

contract_violation::contract_violation(
    const char* comment, contracts::detection_mode detection, bool terminating,
    assertion_kind kind, source_location location,
    evaluation_semantic semantic) noexcept
    : comment_(comment),
      detection_(detection),
      terminating_(terminating),
      kind_(kind),
      location_(location),
      semantic_(semantic) {
}

contract_violation::~contract_violation() {
}

const char* contract_violation::comment() const noexcept {
  return comment_;
}

contracts::detection_mode contract_violation::detection_mode() const noexcept {
  return detection_;
}

bool contract_violation::is_terminating() const noexcept {
  return terminating_;
}

assertion_kind contract_violation::kind() const noexcept {
  return kind_;
}

source_location contract_violation::location() const noexcept {
  return location_;
}

evaluation_semantic contract_violation::semantic() const noexcept {
  return semantic_;
}

void invoke_default_contract_violation_handler(
    const contract_violation& violation) {
  __davecc_default_contract_violation_handler(violation);
}

}  // namespace contracts
}  // namespace std
