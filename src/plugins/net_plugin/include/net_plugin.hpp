#pragma once

#include <iostream>
#include "../../../../lib/appbase/include/plugin.hpp"

class NetPlugin : public appbase::Plugin<NetPlugin> {
public:
  void initialize() override {
    std::cout << "Initialize" << std::endl;
  }

  void start() override {
    std::cout << "Startup" << std::endl;
  }

  void shutdown() override {
    std::cout << "Shutdown" << std::endl;
  }
};
