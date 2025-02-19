#include "source/extensions/filters/http/header_mutation/header_mutation.h"

#include <cstdint>
#include <memory>

#include "source/common/config/utility.h"
#include "source/common/http/header_map_impl.h"
#include "source/common/http/utility.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace HeaderMutation {

void Mutations::mutateRequestHeaders(Http::RequestHeaderMap& request_headers,
                                     const StreamInfo::StreamInfo& stream_info) const {
  request_mutations_.evaluateHeaders(request_headers, request_headers,
                                     *Http::StaticEmptyHeaders::get().response_headers,
                                     stream_info);
}

void Mutations::mutateResponseHeaders(const Http::RequestHeaderMap& request_headers,
                                      Http::ResponseHeaderMap& response_headers,
                                      const StreamInfo::StreamInfo& stream_info) const {
  response_mutations_.evaluateHeaders(response_headers, request_headers, response_headers,
                                      stream_info);
}

PerRouteHeaderMutation::PerRouteHeaderMutation(const PerRouteProtoConfig& config)
    : mutations_(config.mutations()) {}

HeaderMutationConfig::HeaderMutationConfig(const ProtoConfig& config)
    : mutations_(config.mutations()) {}

Http::FilterHeadersStatus HeaderMutation::decodeHeaders(Http::RequestHeaderMap& headers, bool) {
  config_->mutations().mutateRequestHeaders(headers, decoder_callbacks_->streamInfo());

  // Only the most specific route config is used.
  // TODO(wbpcode): It's possible to traverse all the route configs to merge the header mutations
  // in the future.
  route_config_ =
      Http::Utility::resolveMostSpecificPerFilterConfig<PerRouteHeaderMutation>(decoder_callbacks_);

  if (route_config_ != nullptr) {
    route_config_->mutations().mutateRequestHeaders(headers, decoder_callbacks_->streamInfo());
  }

  return Http::FilterHeadersStatus::Continue;
}

Http::FilterHeadersStatus HeaderMutation::encodeHeaders(Http::ResponseHeaderMap& headers, bool) {
  // The request headers will be set to the downstream stream info before the filter chain is
  // started. And in the current implementation, the upstream filter chain will reuse the downstream
  // stream info. So the getRequestHeaders() will never return nullptr no matter the filter is
  // is used as a downstream or upstream filter.
  ASSERT(encoder_callbacks_->streamInfo().getRequestHeaders() != nullptr);
  const auto& request_headers = *encoder_callbacks_->streamInfo().getRequestHeaders();

  config_->mutations().mutateResponseHeaders(request_headers, headers,
                                             encoder_callbacks_->streamInfo());

  if (route_config_ == nullptr) {
    // If we haven't already resolved the route config, do so now.
    route_config_ = Http::Utility::resolveMostSpecificPerFilterConfig<PerRouteHeaderMutation>(
        encoder_callbacks_);
  }

  if (route_config_ != nullptr) {
    route_config_->mutations().mutateResponseHeaders(request_headers, headers,
                                                     encoder_callbacks_->streamInfo());
  }

  return Http::FilterHeadersStatus::Continue;
}

} // namespace HeaderMutation
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
