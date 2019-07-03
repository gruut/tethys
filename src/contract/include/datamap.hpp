#ifndef TETHYS_SCE_DATAMAP_HPP
#define TETHYS_SCE_DATAMAP_HPP

#include <optional>
#include <string>
#include <unordered_map>

namespace tethys::tsce {

class Datamap {
public:
  void set(const std::string &key, const std::string &vv);

  void clear();

  template <typename S = std::string>
  std::optional<std::string> get(S &&key) {
    auto map_it = m_storage.find(key);
    if (map_it == m_storage.end())
      return {};

    return map_it->second;
  }

private:
  std::unordered_map<std::string, std::string> m_storage;
};
} // namespace tethys::tsce

#endif
