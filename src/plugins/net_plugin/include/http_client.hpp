#pragma once

#include <memory>
#include <string>
#include <boost/asio/io_context.hpp>

using namespace std;

namespace gruut {
class HttpClient {
public:
  HttpClient(const string, const string);
  string get(const string, const string);
private:
  string host;
  string port;

  boost::asio::io_context ioc;
};
} // namespace gruut
