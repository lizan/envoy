#pragma once

#include "envoy/server/transport_socket_config.h"

#include "common/config/well_known_names.h"

namespace Envoy {
namespace Server {
namespace Configuration {

/**
 * Config registration for the tcp proxy filter. @see NamedNetworkFilterConfigFactory.
 */
class RawBufferSocketFactory : public virtual TransportSocketConfigFactory {
public:
  virtual ~RawBufferSocketFactory() {}
  std::string name() const override { return Config::TransportSocketNames::get().RAW_BUFFER; }
  Network::TransportSocketFactoryPtr
  createTransportSocketFactory(const Protobuf::Message& config,
                               Network::TransportSocketFactoryContext& context) override;
  ProtobufTypes::MessagePtr createEmptyConfigProto() override;
};

// class DownstreamRawBufferSocketFactory : public DownstreamTransportSocketConfigFactory,
//                                         public RawBufferSocketFactory {};
class UpstreamRawBufferSocketFactory : public UpstreamTransportSocketConfigFactory,
                                       public RawBufferSocketFactory {};

} // namespace Configuration
} // namespace Server
} // namespace Envoy
