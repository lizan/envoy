#pragma once

#include "envoy/server/transport_socket_config.h"

#include "common/config/well_known_names.h"

namespace Envoy {
namespace Server {
namespace Configuration {

/**
 * Config registration for the tcp proxy filter. @see NamedNetworkFilterConfigFactory.
 */
class SslSocketConfigFactory : public virtual TransportSocketConfigFactory {
public:
  virtual ~SslSocketConfigFactory() {}
  std::string name() const override { return Config::TransportSocketNames::get().BORINGSSL; }
};
/*
class DownstreamSslSocketFactory : public DownstreamTransportSocketConfigFactory,
                                   public SslSocketConfigFactory {
 public:
  Network::TransportSocketFactoryPtr createTransportSocketFactory(const Protobuf::Message &config)
override; ProtobufTypes::MessagePtr createEmptyConfigProto() override;
};
*/
class UpstreamSslSocketFactory : public UpstreamTransportSocketConfigFactory,
                                 public SslSocketConfigFactory {
public:
  Network::TransportSocketFactoryPtr
  createTransportSocketFactory(const Protobuf::Message& config,
                               Network::TransportSocketFactoryContext& context) override;
  ProtobufTypes::MessagePtr createEmptyConfigProto() override;
};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
