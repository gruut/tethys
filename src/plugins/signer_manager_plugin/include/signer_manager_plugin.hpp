#pragma once

#include <boost/filesystem.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../../../../lib/log/include/log.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "plugin.hpp"

using namespace appbase;

namespace gruut {
class SignerManagerPlugin : public Plugin<SignerManagerPlugin> {
public:
  PLUGIN_REQUIRES()

  SignerManagerPlugin();

  ~SignerManagerPlugin() override;

  void pluginInitialize(const boost::program_options::variables_map &options);

  void pluginStart();

  void pluginShutdown() {
    logger::INFO("SignerManagerPlugin Shutdown");
  }

  void setProgramOptions(options_description &cfg) override;

private:
  std::unique_ptr<class SignerManagerPluginImpl> impl;
};
} // namespace gruut
