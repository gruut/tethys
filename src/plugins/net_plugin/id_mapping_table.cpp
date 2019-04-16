#include "include/id_mapping_table.hpp"

namespace gruut::net_plugin {
void IdMappingTable::mapId(const b58_user_id_type &b58_user_id, const net_id_type &net_id) {
  {
    std::lock_guard<std::mutex> guard(id_map_mutex);
    id_mapping_table.try_emplace(b58_user_id, Hash<160>::sha1(net_id));
  }
}

void IdMappingTable::unmapId(const b58_user_id_type &b58_user_id) {
  if (id_mapping_table.count(b58_user_id) > 0) {
    {
      std::lock_guard<std::mutex> guard(id_map_mutex);
      id_mapping_table.erase(b58_user_id);
    }
  }
}

void IdMappingTable::unmapId(const hashed_net_id_type &dead_hashed_id) {
  for (auto &[b58_user_id, net_hased_id] : id_mapping_table) {
    if (net_hased_id == dead_hashed_id) {
      unmapId(b58_user_id);
      return;
    }
  }
}

std::optional<hashed_net_id_type> IdMappingTable::get(const b58_user_id_type &id) {
  if (id_mapping_table.count(id) > 0) {
    return id_mapping_table[id];
  }

  return {};
}

} // namespace gruut
