#include "TVirtualProtocol.h"

namespace apache { namespace thrift { namespace protocol {

TProtocolDefaults::TProtocolDefaults(std::shared_ptr<TTransport> ptrans)
  : TProtocol(ptrans)
{}

}}}
