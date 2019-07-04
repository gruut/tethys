#ifndef TETHYS_SCE_GET_HANDLER_HPP
#define TETHYS_SCE_GET_HANDLER_HPP

#include "../condition_manager.hpp"
#include "../data_manager.hpp"

namespace tethys::tsce {

class GetHandler {
public:
  GetHandler() = default;

  void parseGet(std::vector<std::pair<tinyxml2::XMLElement *, std::string>> &get_nodes, ConditionManager &condition_manager,
                DataManager &data_storage);
};

} // namespace tethys::tsce

#endif
