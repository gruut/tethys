#include "include/http_client.hpp"
#include "../../../lib/log/include/log.hpp"

#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

using namespace std;
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

namespace tethys {
tethys::HttpClient::HttpClient(const string &_host, const string &_port) : host(_host), port(_port) {}

string HttpClient::get(const string &target, const string &query) {
  try {
    tcp::resolver resolver{ioc};
    tcp::socket socket{ioc};

    auto const results = resolver.resolve(host, port);

    boost::asio::connect(socket, results.begin(), results.end());

    http::request<http::string_body> req;
    req.method(http::verb::get);
    req.target(target + "?" + encodeQuery(query));
    req.set(http::field::host, host);
    req.set(http::field::content_type, "application/x-www-form-urlencoded");

    req.body() = query;
    req.prepare_payload();

    http::write(socket, req);

    boost::beast::flat_buffer buffer;

    http::response<http::dynamic_body> res;

    http::read(socket, buffer, res);

    string body{boost::asio::buffers_begin(res.body().data()), boost::asio::buffers_end(res.body().data())};

    return body;
  } catch (exception const &e) {
    logger::ERROR("Failed to request with GET method: {}", e.what());

    return "";
  }
}

string HttpClient::encodeQuery(const string &value) {
  ostringstream escaped;
  escaped.fill('0');
  escaped << hex;

  for (auto character : value) {
    string::value_type c = character;

    // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }
    // Any other characters are percent-encoded
    escaped << uppercase;
    escaped << '%' << setw(2) << int((unsigned char)c);
    escaped << nouppercase;
  }
  return escaped.str();
}

} // namespace tethys
