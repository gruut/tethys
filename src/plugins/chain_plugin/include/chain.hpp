#pragma once

#include "../../../../include/json.hpp"
#include <boost/program_options/variables_map.hpp>
#include <memory>

namespace gruut {

using namespace std;

class Chain {
public:
  Chain();
  ~Chain();

  Chain(const Chain &) = delete;
  Chain(const Chain &&) = delete;
  Chain &operator=(const Chain &) = delete;

  void startup(nlohmann::json &genesis_state);

private:
  unique_ptr<class ChainImpl> impl;
};
} // namespace gruut
