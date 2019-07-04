#include "include/handler/certificate_handler.hpp"
#include "../../lib/tethys-utils/src/cert_verifier.hpp"
#include "include/config.hpp"
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/ecdsa.h>
#include <botan-2/botan/oids.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/x509cert.h>

namespace tethys::tsce {

bool tsce::CertificateHandler::evalue(tinyxml2::XMLElement *doc_node, DataManager &data_manager) {
  if (doc_node == nullptr)
    return false;

  auto pk_node = doc_node->FirstChildElement("pk");
  auto by_node = doc_node->FirstChildElement("by");

  if (pk_node == nullptr || by_node == nullptr)
    return false;

  std::string pk = mt::c2s(pk_node->Attribute("value"));
  std::string pk_type = mt::c2s(pk_node->Attribute("type"));

  if (pk.empty()) {
    return false;
  } else {
    pk = data_manager.eval(pk);
    if (pk.empty())
      return false;
  }

  std::string by_attr_val = mt::c2s(by_node->Attribute("value"));
  std::string by_type = mt::c2s(by_node->Attribute("type"));
  std::string by_val = !by_attr_val.empty() ? data_manager.eval(by_attr_val) : mt::c2s(by_node->GetText());

  if (by_val.empty())
    return false;

  // TODO : may be handled by type ( `PEM` / `DER` )
  // now just check `PEM`
  return CertVerifier::doVerify(pk, by_val);
}
}