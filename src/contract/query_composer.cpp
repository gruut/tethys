#include "include/query_composer.hpp"
#include "../../lib/json/include/json.hpp"
#include "include/config.hpp"

using namespace std;

namespace tethys::tsce {
nlohmann::json tsce::QueryComposer::compose(std::vector<nlohmann::json> &result_queries, const std::string &block_id, uint64_t height) {
  nlohmann::json result;
  result["block"]["id"] = block_id;
  result["block"]["height"] = to_string(height);

  result["results"] = nlohmann::json::array();

  for (auto &res : result_queries) {
    if (json::get<bool>(res, "status").value())
      res.erase("info");
    else {
      res.erase("authority");
      // res.erase("fee");
      res.erase("queries");

      res["fee"]["author"] = 0;
      res["fee"]["user"] = MIN_USER_FEE;
    }
    result["results"].emplace_back(res);
  }
  return result;
}
} // namespace tethys::tsce
