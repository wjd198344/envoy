#include "cxx_swift_interop.h"

#include "source/server/options_impl.h"

#include "library/common/engine.h"

namespace Envoy {
namespace CxxSwift {

void run(BootstrapPtr bootstrap_ptr, Platform::LogLevel log_level, envoy_engine_t engine_handle) {
  auto options = std::make_unique<Envoy::OptionsImpl>();
  auto bootstrap =
      absl::WrapUnique(reinterpret_cast<envoy::config::bootstrap::v3::Bootstrap*>(bootstrap_ptr));
  options->setConfigProto(std::move(bootstrap));
  options->setLogLevel(static_cast<spdlog::level::level_enum>(log_level));
  options->setConcurrency(1);
  reinterpret_cast<Envoy::Engine*>(engine_handle)->run(std::move(options));
}

} // namespace CxxSwift
} // namespace Envoy
