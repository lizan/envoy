#include "async_client_impl.h"
#include "headers.h"

namespace Http {

const std::vector<std::reference_wrapper<const Router::RateLimitPolicyEntry>>
    AsyncStreamingRequestImpl::NullRateLimitPolicy::rate_limit_policy_entry_;
const AsyncStreamingRequestImpl::NullRateLimitPolicy
    AsyncStreamingRequestImpl::RouteEntryImpl::rate_limit_policy_;
const AsyncStreamingRequestImpl::NullRetryPolicy
    AsyncStreamingRequestImpl::RouteEntryImpl::retry_policy_;
const AsyncStreamingRequestImpl::NullShadowPolicy
    AsyncStreamingRequestImpl::RouteEntryImpl::shadow_policy_;
const AsyncStreamingRequestImpl::NullVirtualHost
    AsyncStreamingRequestImpl::RouteEntryImpl::virtual_host_;

AsyncClientImpl::AsyncClientImpl(const Upstream::ClusterInfo& cluster, Stats::Store& stats_store,
                                 Event::Dispatcher& dispatcher,
                                 const LocalInfo::LocalInfo& local_info,
                                 Upstream::ClusterManager& cm, Runtime::Loader& runtime,
                                 Runtime::RandomGenerator& random,
                                 Router::ShadowWriterPtr&& shadow_writer)
    : cluster_(cluster), config_("http.async-client.", local_info, stats_store, cm, runtime, random,
                                 std::move(shadow_writer), true),
      dispatcher_(dispatcher) {}

AsyncClientImpl::~AsyncClientImpl() {
  while (!active_requests_.empty()) {
    active_requests_.front()->failDueToClientDestroy();
  }
}

AsyncClient::Request* AsyncClientImpl::send(MessagePtr&& request, AsyncClient::Callbacks& callbacks,
                                            const Optional<std::chrono::milliseconds>& timeout) {
  AsyncRequestImpl *async_request = new AsyncRequestImpl(std::move(request), *this, callbacks, timeout);
  std::unique_ptr<AsyncStreamingRequestImpl> new_request{async_request};

  // The request may get immediately failed. If so, we will return nullptr.
  if (!new_request->complete_) {
    new_request->moveIntoList(std::move(new_request), active_requests_);
    return async_request;
  } else {
    return nullptr;
  }
}

AsyncClient::StreamingRequest* AsyncClientImpl::start(AsyncClient::StreamingCallbacks& callbacks,
                                                      const Optional<std::chrono::milliseconds>& timeout) {
  AsyncStreamingRequestImpl *async_request = new AsyncStreamingRequestImpl(*this, callbacks, timeout);
  std::unique_ptr<AsyncStreamingRequestImpl> new_request{async_request};

  // The request may get immediately failed. If so, we will return nullptr.
  if (!new_request->complete_) {
    new_request->moveIntoList(std::move(new_request), active_requests_);
    return async_request;
  } else {
    return nullptr;
  }
}

AsyncStreamingRequestImpl::AsyncStreamingRequestImpl(
    AsyncClientImpl& parent, AsyncClient::StreamingCallbacks& callbacks,
    const Optional<std::chrono::milliseconds>& timeout)
    : parent_(parent), streaming_callbacks_(callbacks),
      stream_id_(parent.config_.random_.random()), router_(parent.config_),
      request_info_(Protocol::Http11), route_(parent_.cluster_.name(), timeout) {

  router_.setDecoderFilterCallbacks(*this);
  // TODO: Correctly set protocol in request info when we support access logging.
}

AsyncStreamingRequestImpl::~AsyncStreamingRequestImpl() { ASSERT(!reset_callback_); }

void AsyncStreamingRequestImpl::encodeHeaders(HeaderMapPtr&& headers, bool end_stream) {
#ifndef NDEBUG
  log_debug("async http request response headers (end_stream={}):", end_stream);
  headers->iterate([](const HeaderEntry& header, void*) -> void {
    log_debug("  '{}':'{}'", header.key().c_str(), header.value().c_str());
  }, nullptr);
#endif

  streaming_callbacks_.onHeaders(std::move(headers), end_stream);
}

void AsyncStreamingRequestImpl::encodeData(Buffer::Instance& data, bool end_stream) {
  log_trace("async http request response data (length={} end_stream={})", data.length(),
            end_stream);
  streaming_callbacks_.onData(data, end_stream);
}

void AsyncStreamingRequestImpl::encodeTrailers(HeaderMapPtr&& trailers) {
#ifndef NDEBUG
  log_debug("async http request response trailers:");
  trailers->iterate([](const HeaderEntry& header, void*) -> void {
    log_debug("  '{}':'{}'", header.key().c_str(), header.value().c_str());
  }, nullptr);
#endif

  streaming_callbacks_.onTrailers(std::move(trailers));
}

/*void AsyncStreamingRequestImpl::sendHeaders(HeaderMapPtr &&headers, bool end_stream) {}
void AsyncStreamingRequestImpl::sendData(Buffer::Instance &data, bool end_stream) {}
void AsyncStreamingRequestImpl::sendTrailers(HeaderMapPtr &&trailers) {}
void AsyncStreamingRequestImpl::sendResetStream() {}
*/

void AsyncStreamingRequestImpl::cleanup() {
  // response_.reset();
  reset_callback_ = nullptr;

  // This will destroy us, but only do so if we are actually in a list. This does not happen in
  // the immediate failure case.
  if (inserted()) {
    removeFromList(parent_.active_requests_);
  }
}

void AsyncStreamingRequestImpl::resetStream() {
  streaming_callbacks_.onResetStream();
  cleanup();
}


void AsyncStreamingRequestImpl::failDueToClientDestroy() {
  // In this case we are going away because the client is being destroyed. We need to both reset
  // the stream as well as raise a failure callback.
  reset_callback_();
  resetStream();
}

AsyncRequestImpl::AsyncRequestImpl(MessagePtr &&request,
                                   AsyncClientImpl &parent,
                                   AsyncClient::Callbacks &callbacks,
                                   const Optional<std::chrono::milliseconds> &timeout)
    : AsyncStreamingRequestImpl(parent, *this, timeout), request_(std::move(request)),
      callbacks_(callbacks) {

  sendHeaders(request_->headers(), !request_->body());
  if (!complete_ && request_->body()) {
    sendData(*request_->body(), true);
  }
  // TODO: Support request trailers.
}

AsyncRequestImpl::~AsyncRequestImpl() {}

void AsyncRequestImpl::onComplete() {
  complete_ = true;
  callbacks_.onSuccess(std::move(response_));
  cleanup();
}

void AsyncRequestImpl::onHeaders(HeaderMapPtr &&headers, bool end_stream) {
  response_.reset(new ResponseMessageImpl(std::move(headers)));

  if (end_stream) {
    onComplete();
  }
}

void AsyncRequestImpl::onData(Buffer::Instance &data, bool end_stream) {
  if (!response_->body()) {
    response_->body(Buffer::InstancePtr{new Buffer::OwnedImpl()});
  }
  response_->body()->move(data);

  if (end_stream) {
    onComplete();
  }
}

void AsyncRequestImpl::onTrailers(HeaderMapPtr &&trailers) {
  response_->trailers(std::move(trailers));
  onComplete();
}

void AsyncRequestImpl::onResetStream() {
  // In this case we don't have a valid response so we do need to raise a failure.
  callbacks_.onFailure(AsyncClient::FailureReason::Reset);
}

void AsyncRequestImpl::cancel() {
  sendResetStream();
}


} // Http
