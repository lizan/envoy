#pragma once

#include "envoy/secret/secret_manager.h"
#include "envoy/upstream/cluster_manager.h"

#include "common/http/context_impl.h"
#include "common/upstream/cluster_manager_impl.h"

#include "server/config_validation/async_client.h"

namespace Envoy {
namespace Upstream {

/**
 * Config-validation-only implementation of ClusterManagerFactory, which creates
 * ValidationClusterManagers. It also creates, but never returns, CdsApiImpls.
 */
class ValidationClusterManagerFactory : public ProdClusterManagerFactory {
public:
  using ProdClusterManagerFactory::ProdClusterManagerFactory;

  ClusterManagerPtr
  clusterManagerFromProto(const envoy::config::bootstrap::v2::Bootstrap& bootstrap) override;

  // Delegates to ProdClusterManagerFactory::createCds, but discards the result and returns nullptr
  // unconditionally.
  CdsApiPtr createCds(const envoy::api::v2::core::ConfigSource& cds_config,
                      ClusterManager& cm) override;
};

/**
 * Config-validation-only implementation of ClusterManager, which opens no upstream connections.
 */
class ValidationClusterManager : public ClusterManagerImpl {
public:
  ValidationClusterManager(const envoy::config::bootstrap::v2::Bootstrap& bootstrap,
                           ClusterManagerFactory& factory, Stats::Store& stats,
                           ThreadLocal::Instance& tls, Runtime::Loader& runtime,
                           Runtime::RandomGenerator& random, const LocalInfo::LocalInfo& local_info,
                           AccessLog::AccessLogManager& log_manager, Event::Dispatcher& dispatcher,
                           Server::Admin& admin, Api::Api& api, Http::Context& http_context);

  Http::ConnectionPool::Instance* httpConnPoolForCluster(const std::string&, ResourcePriority,
                                                         Http::Protocol,
                                                         LoadBalancerContext*) override;
  Host::CreateConnectionData tcpConnForCluster(const std::string&, LoadBalancerContext*,
                                               Network::TransportSocketOptionsSharedPtr) override;
  Http::AsyncClient& httpAsyncClientForCluster(const std::string&) override;

private:
  // ValidationAsyncClient always returns null on send() and start(), so it has
  // no internal state -- we might as well just keep one and hand out references
  // to it.
  Http::ValidationAsyncClient async_client_;
};

} // namespace Upstream
} // namespace Envoy
