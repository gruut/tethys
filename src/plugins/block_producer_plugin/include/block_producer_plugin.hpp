#pragma once

#include <iostream>
#include "../../../../lib/appbase/include/plugin.hpp"

class BlockProducerPlugin : public appbase::Plugin<BlockProducerPlugin> {
public:
  void initialize() override {
    std::cout << "BlockProducerPlugin Initialize" << std::endl;
  }

  void start() override {
    std::cout << "Startup" << std::endl;
  }

  void shutdown() override {
    std::cout << "Shutdown" << std::endl;
  }
};
