#ifndef TETHYS_SCE_QUERY_COMPOSER_HPP
#define TETHYS_SCE_QUERY_COMPOSER_HPP

#include "config.hpp"


namespace tethys::tsce {

class QueryComposer {
public:
  QueryComposer() = default;

  nlohmann::json compose(std::vector<nlohmann::json> &result_queries, const std::string &block_id, uint64_t height) {
    nlohmann::json result;
    result["block"]["id"] = block_id;
    result["block"]["height"] = to_string(height);

    result["results"] = nlohmann::json::array();

    for(auto &res : result_queries){
      if(JsonTool::get<bool>(res, "status").value())
        res.erase("info");
      else{
        res.erase("authority");
        //res.erase("fee");
        res.erase("queries");

        res["fee"]["author"] = 0;
        res["fee"]["user"] = MIN_USER_FEE;
      }
      result["results"].emplace_back(res);
    }
    return result;
  }
};
}

#endif
