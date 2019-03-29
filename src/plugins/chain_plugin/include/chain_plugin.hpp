#pragma once

#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "application.hpp"
#include "plugin.hpp"

using namespace appbase;

namespace gruut {
  class ChainPlugin : public Plugin<ChainPlugin> {
  public:
    PLUGIN_REQUIRES()

    void pluginInitialize(const boost::program_options::variables_map &options) {
      logger::INFO("ChainPlugin Initialize");
    }

    void pluginStart() {
      logger::INFO("ChainPlugin Start");
    }

    void pluginShutdown() {
      logger::INFO("ChainPlugin Shutdown");
    }

  private:
  };
} // namespace gruut
