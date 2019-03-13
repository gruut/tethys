#pragma once

#include "../include/curlpp.hpp"
#include "../include/nlohmann/json.hpp"

namespace gruut{
namespace net{

class HttpClient {
public:
  HttpClient() = default;

  CURLcode post(const std::string &url_addr, const std::string &json_dump);
  CURLcode postAndGetReply(const std::string &url_addr, const std::string &json_dump, nlohmann::json &response_json);

private:
  std::string getPostField(const std::string &key, const std::string &value);
  static size_t writeCallback(const char *in, size_t size, size_t num,
							  std::string *out);

  curlpp::Easy m_curl;
};

}
}