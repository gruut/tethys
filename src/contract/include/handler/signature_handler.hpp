#ifndef TETHYS_SCE_HANDLER_SIGNATURE_HPP
#define TETHYS_SCE_HANDLER_SIGNATURE_HPP

#include "base_condition_handler.hpp"
#include "../xml_tool.hpp"
#include "../../../../lib/tethys-utils/src/bytes_builder.hpp"

namespace tethys::tsce {

  class SignatureHandler : public BaseConditionHandler {
  public:
    SignatureHandler() = default;

    bool evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) override;

  private:
    void appendData(const std::string &type, const std::string &data, BytesBuilder &bytes_builder);

    bool verifySig(std::string_view signature_type,
                   const std::string &signature,
                   std::string_view pk_type,
                   const string &pk_or_pem,
                   const std::string &msg);
  };

}

#endif
