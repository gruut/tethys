#ifndef GRUUT_PUBLIC_MERGER_ENDORSER_HPP
#define GRUUT_PUBLIC_MERGER_ENDORSER_HPP

#include "../config/storage_type.hpp"

namespace gruut {
class Endorser {
public:
  base58_type endorser_id;
  string endorser_pk;
  base64_type endorser_signature;

  Endorser() = default;
  Endorser(base58_type &&endorser_id_, string &&endorser_pk_, base64_type &&endorser_signature_)
      : endorser_id(endorser_id_), endorser_pk(endorser_pk_), endorser_signature(endorser_signature_) {}
  Endorser(base58_type &endorser_id_, string &endorser_pk_, base64_type &endorser_signature_)
      : endorser_id(endorser_id_), endorser_pk(endorser_pk_), endorser_signature(endorser_signature_) {}
};
} // namespace gruut

#endif // GRUUT_PUBLIC_MERGER_ENDORSER_HPP
