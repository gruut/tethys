#ifndef TETHYS_SCE_DATAMAP_HPP
#define TETHYS_SCE_DATAMAP_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include "config.hpp"

namespace tethys::tsce {

class Datamap {
private:
  std::unordered_map<std::string, std::string> m_storage;

public:

  void set(const std::string &key, const std::string &vv) {
    if (key.empty())
      return;

    auto ret = m_storage.insert({key, vv}); // as new
    if (!ret.second) { // as update
      ret.first->second = vv;
    }
  }

  template <typename S = std::string>
  std::optional<std::string> get(S &&key) {
    auto map_it = m_storage.find(key);
    if(map_it == m_storage.end())
      return {};

    return map_it->second;
  }

  void clear(){
    m_storage.clear();
  }


};

}

#endif
