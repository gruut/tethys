#pragma once

#include <iostream>

#include "../../../../lib/appbase/include/application.hpp"
#include "../../../../lib/appbase/include/plugin.hpp"
#include "../../../../lib/appbase/include/channel_interface.hpp"

using namespace appbase;

class BlockProducerPlugin : public Plugin<BlockProducerPlugin> {
public:
  BlockProducerPlugin() : temp_channel(app().get_channel<channels::temp_channel::channel_type>()) {}

  void initialize() override {
    std::cout << "BlockProducerPlugin Initialize" << std::endl;
  }

  void start() override {
    std::cout << "Startup" << std::endl;

    TempData d;
    temp_channel.publish(d);
  }

  void shutdown() override {
    std::cout << "Shutdown" << std::endl;
  }

private:
  channels::temp_channel::channel_type &temp_channel;
};
