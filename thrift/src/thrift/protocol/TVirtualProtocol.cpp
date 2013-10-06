#include "TVirtualProtocol.h"

namespace apache { namespace thrift { namespace protocol {

TProtocolDefaults::TProtocolDefaults(boost::shared_ptr<TTransport> ptrans)
  : TProtocol(ptrans)
{}

}}}
