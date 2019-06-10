#include <string>

#include "include/datamap.hpp"

namespace tethys::tsce {

  void Datamap::set(const std::string &key, const std::string &vv) {
    if (key.empty())
      return;

    auto ret = m_storage.insert({key, vv}); // as new
    if (!ret.second) { // as update
      ret.first->second = vv;
    }
  }

  void Datamap::clear() {
    m_storage.clear();
  }
}