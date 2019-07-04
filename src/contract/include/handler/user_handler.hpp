#ifndef TETHYS_SCE_HANDLER_USER_HPP
#define TETHYS_SCE_HANDLER_USER_HPP

#include "base_condition_handler.hpp"
#include <vector>

namespace tethys::tsce {

class UserHandler : public BaseConditionHandler {
private:
  std::string m_user_key;

public:
  UserHandler();

  void setUserType(PrimaryConditionType user_type);

  bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;
};

} // namespace tethys::tsce

#endif
