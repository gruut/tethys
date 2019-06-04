#pragma once

#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>

using namespace std;

namespace tethys {
class HttpClient {
public:
  HttpClient(const string, const string);
  string get(const string, const string);

private:
  string host;
  string port;

  boost::asio::io_context ioc;
};
} // namespace tethys
