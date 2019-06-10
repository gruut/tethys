#include "include/handler/certificate_handler.hpp"
#include <botan-2/botan/oids.h>
#include <botan-2/botan/rsa.h>
#include <botan-2/botan/data_src.h>
#include <botan-2/botan/x509cert.h>
#include <botan-2/botan/ecdsa.h>
#include "include/config.hpp"

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

    //TODO : may be handled by type ( `PEM` / `DER` )
    //now just check `PEM`
    try {
      Botan::DataSource_Memory by_cert_datasource(by_val);
      Botan::DataSource_Memory cert_datasource(pk);

      Botan::X509_Certificate by_cert(by_cert_datasource);
      Botan::X509_Certificate cert(cert_datasource);

      std::string algo_name = Botan::OIDS::oid2str(by_cert.subject_public_key_algo().get_oid());

      if (algo_name.size() < 3) {
        return false;
      }

      ssize_t pos;
      if ((pos = algo_name.find("RSA")) != std::string::npos && pos == 0) {
        Botan::RSA_PublicKey by_pk(by_cert.subject_public_key_algo(), by_cert.subject_public_key_bitstring());
        return cert.check_signature(by_pk);
      } else if ((pos = algo_name.find("ECDSA")) != std::string::npos && pos == 0) {
        Botan::ECDSA_PublicKey by_pk(by_cert.subject_public_key_algo(), by_cert.subject_public_key_bitstring());
        return cert.check_signature(by_pk);
      } else {
        return false; // not supported!
      }
    }
    catch (...) {
      return false;
    }
  }
}