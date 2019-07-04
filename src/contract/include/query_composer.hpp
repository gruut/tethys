#ifndef TETHYS_SCE_QUERY_COMPOSER_HPP
#define TETHYS_SCE_QUERY_COMPOSER_HPP

#include "../../../include/json.hpp"

namespace tethys::tsce {

class QueryComposer {
public:
  QueryComposer() = default;

  nlohmann::json compose(std::vector<nlohmann::json> &result_queries, const std::string &block_id, uint64_t height);
};
} // namespace tethys::tsce

#endif
