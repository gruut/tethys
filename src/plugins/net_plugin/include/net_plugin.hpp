#pragma once

#include <iostream>

#include "application.hpp"
#include "plugin.hpp"
#include "channel_interface.hpp"

using namespace appbase;

class NetPlugin : public Plugin<NetPlugin> {
public:
  void initialize() override {
    std::cout << "NetPlugin Initialize" << std::endl;

    temp_channel_handler = app().get_channel<channels::temp_channel::channel_type>().subscribe([](TempData d) {
      std::cout << "NetPlugin handler" << std::endl;
    });
  }

  void start() override {
    std::cout << "Startup" << std::endl;
  }

  void shutdown() override {
    std::cout << "Shutdown" << std::endl;
  }

private:
  channels::temp_channel::channel_type::Handle temp_channel_handler;
};
