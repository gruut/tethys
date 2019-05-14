#pragma once

#include <boost/program_options/options_description.hpp>
#include <iostream>
#include <memory>

#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../../net_plugin/include/net_plugin.hpp"
#include "application.hpp"
#include "plugin.hpp"

using namespace appbase;
using namespace boost::program_options;

namespace gruut {

class AdminPlugin : public Plugin<AdminPlugin> {
public:
  PLUGIN_REQUIRES()

  AdminPlugin();

  ~AdminPlugin() override;

  void pluginInitialize(const variables_map &options);

  void pluginStart();
  void pluginShutdown() {
    logger::INFO("AdminPlugin Shutdown");
  }
  void setProgramOptions(po::options_description &cfg) override;

private:
  std::unique_ptr<class AdminPluginImpl> impl;
};
} // namespace gruut
