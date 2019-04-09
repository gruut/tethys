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

using b58_user_id_type = std::string;
using net_id_type = std::string;    // Network virtual id type
using hashed_net_id_type = Hash160; // Network hashed virtual id type

using BroadcastMsgTable = std::unordered_map<std::string, uint64_t>;
} // namespace net_plugin
} // namespace gruut
