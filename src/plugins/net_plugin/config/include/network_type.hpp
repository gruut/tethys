#pragma once

#include "../../kademlia/include/hash.hpp"
#include <string>
#include <unordered_map>

namespace gruut {
namespace net_plugin {
struct IpEndpoint {
  std::string address;
  std::string port;

  bool operator==(IpEndpoint &rhs) const {
    return (this->address == rhs.address) && (this->port == rhs.port);
  }
};

using IdType = std::string;   // 32bytes ID
using HashedIdType = Hash160; // 32bytes ID를 hash160

using BroadcastMsgTable = std::unordered_map<std::string, uint64_t>;
} // namespace net_plugin
} // namespace gruut
