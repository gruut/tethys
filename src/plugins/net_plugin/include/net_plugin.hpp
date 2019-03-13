#pragma once

#include <iostream>

#include "application.hpp"
#include "plugin.hpp"
#include "channel_interface.hpp"

#include "../../../../lib/log/include/log.hpp"

using namespace appbase;

class NetPlugin : public Plugin<NetPlugin> {
public:
  PLUGIN_REQUIRES()

  void plugin_initialize() {
    logger::INFO("NetPlugin Initialize");

    temp_channel_handler = app().get_channel<channels::temp_channel::channel_type>().subscribe([](TempData d) {
      std::cout << "NetPlugin handler" << std::endl;
    });
  }

  void plugin_start() {
    logger::INFO("NetPlugin Startup");
  }

  void plugin_shutdown() {
    logger::INFO("NetPlugin Shutdown");
  }

private:
  channels::temp_channel::channel_type::Handle temp_channel_handler;
};
