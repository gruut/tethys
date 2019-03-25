#pragma once

#include <iostream>
#include <memory>

#include "application.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "plugin.hpp"

#include "../../../../lib/log/include/log.hpp"

using namespace appbase;
using namespace boost::program_options;

namespace gruut {
  class NetPlugin : public Plugin<NetPlugin> {
  public:
    PLUGIN_REQUIRES()

    NetPlugin();

    ~NetPlugin() override;

    void pluginInitialize(const variables_map& options);

    void pluginStart();

    void pluginShutdown() {
      logger::INFO("NetPlugin Shutdown");
    }

  private:
    std::unique_ptr<class NetPluginImpl> impl;
    outgoing::channels::network::channel_type::Handle out_channel_handler;
  };
}
