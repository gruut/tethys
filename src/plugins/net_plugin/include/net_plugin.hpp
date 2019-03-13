#pragma once

#include <iostream>
#include <memory>

#include "application.hpp"
#include "plugin.hpp"
#include "channel_interface.hpp"

#include "../../../../lib/log/include/log.hpp"

using namespace appbase;

class NetPluginImpl;

class NetPlugin : public Plugin<NetPlugin> {
public:
  PLUGIN_REQUIRES()

  NetPlugin();

  void plugin_initialize();

  void plugin_start() {
    logger::INFO("NetPlugin Startup");
  }

  void plugin_shutdown() {
    logger::INFO("NetPlugin Shutdown");
  }

private:
  std::unique_ptr<NetPluginImpl> impl;
  channels::temp_channel::channel_type::Handle temp_channel_handler;
};
