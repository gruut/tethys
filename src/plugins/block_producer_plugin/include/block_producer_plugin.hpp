#pragma once

#include <iostream>

#include "application.hpp"
#include "plugin.hpp"
#include "channel_interface.hpp"
#include "../../net_plugin/include/net_plugin.hpp"
#include "../../../../lib/log/include/log.hpp"

using namespace appbase;

namespace gruut {
  class BlockProducerPlugin : public Plugin<BlockProducerPlugin> {
  public:
    PLUGIN_REQUIRES((NetPlugin))

    BlockProducerPlugin() : temp_channel(app().get_channel<channels::temp_channel::channel_type>()) {}

    void plugin_initialize() {
      logger::INFO("BlockProducerPlugin Initialize");
    }

    void plugin_start() {
      logger::INFO("BlockProducerPlugin Startup");

      TempData d;
      temp_channel.publish(d);
    }

    void plugin_shutdown() {
      logger::INFO("BlockProducerPlugin Shutdown");
    }

  private:
    channels::temp_channel::channel_type &temp_channel;
  };
}
