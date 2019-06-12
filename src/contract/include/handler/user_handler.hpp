#ifndef TETHYS_SCE_HANDLER_USER_HPP
#define TETHYS_SCE_HANDLER_USER_HPP

#include <vector>
#include "base_condition_handler.hpp"

namespace tethys::tsce {

class UserHandler : public BaseConditionHandler {
private:
  std::string m_user_key;
public:
  UserHandler();

  void setUserType(PrimaryConditionType user_type);

  bool evalue(tinyxml2::XMLElement* doc_node, DataManager &data_manager) override;

};

}


#endif
