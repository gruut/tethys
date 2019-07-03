#ifndef TETHYS_SCE_HANDLER_CERTIFICATE_HPP
#define TETHYS_SCE_HANDLER_CERTIFICATE_HPP

#include "base_condition_handler.hpp"

namespace tethys::tsce {

class CertificateHandler : public BaseConditionHandler {
public:
  CertificateHandler() = default;

  bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;
};

} // namespace tethys::tsce

#endif
