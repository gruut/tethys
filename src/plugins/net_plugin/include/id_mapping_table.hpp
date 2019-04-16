#pragma once

#include <mutex>
#include <unordered_map>

#include "../config/include/network_type.hpp"
#include "../kademlia/include/node.hpp"

namespace gruut::net_plugin {
class IdMappingTable {
public:
  void mapId(const b58_user_id_type &b58_user_id, const net_id_type &net_id);
  void unmapId(const b58_user_id_type &b58_user_id);
  void unmapId(const hashed_net_id_type &hashed_net_id);

  std::optional<hashed_net_id_type> get(const b58_user_id_type&);
private:
  std::unordered_map<b58_user_id_type, hashed_net_id_type> id_mapping_table;
  std::mutex id_map_mutex;
};
} // namespace gruut
