#ifndef GRUUT_PUBLIC_MERGER_SIGNATURE_HPP
#define GRUUT_PUBLIC_MERGER_SIGNATURE_HPP

#include "../config/storage_type.hpp"

namespace tethys {
class Signature {
public:
  base58_type signer_id;
  base64_type signer_sig;

  Signature() = default;
  Signature(base58_type &&signer_id_, base64_type &&signer_sig_) : signer_id(signer_id_), signer_sig(signer_sig_) {}
  Signature(base58_type &signer_id_, base64_type &signer_sig_) : signer_id(signer_id_), signer_sig(signer_sig_) {}
};
} // namespace tethys
#endif