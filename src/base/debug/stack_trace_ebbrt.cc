
#include "src/base/debug/stack_trace.h"
#include <src/base/platform/platform.h>


namespace v8 {
namespace base {
namespace debug {

bool EnableInProcessStackDumping() {
  return false;
}

void DisableSignalStackDump() {
}

StackTrace::StackTrace() {
}

void StackTrace::Print() const {
  std::string backtrace = ToString();
  OS::Print("%s\n", backtrace.c_str());
}

void StackTrace::OutputToStream(std::ostream* os) const {
}

}  // namespace debug
}  // namespace base
}  // namespace v8
