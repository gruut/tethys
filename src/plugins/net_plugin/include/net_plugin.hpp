#pragma once

#include <iostream>
#include <memory>

#include "application.hpp"
#include "channel_interface.hpp"
#include "plugin.hpp"

#include "../../../../lib/log/include/log.hpp"

using namespace appbase;

namespace gruut {
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
    //std::unique_ptr<class NetPluginImpl> impl;
    channels::temp_channel::channel_type::Handle temp_channel_handler;
  };
}
