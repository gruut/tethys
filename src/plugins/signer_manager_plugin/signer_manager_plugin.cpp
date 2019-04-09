#include "include/signer_manager_plugin.hpp"
#include "../../../include/json.hpp"
#include "../../../lib/log/include/log.hpp"
#include "include/signer_pool.hpp"

namespace gruut {

using namespace std;

namespace fs = boost::filesystem;

class SignerManagerPluginImpl {
public:
  unique_ptr<SignerPool> signer_pool;
  void initialize() {
    signer_pool = make_unique<SignerPool>();
  }
};

SignerManagerPlugin::SignerManagerPlugin() : impl(new SignerManagerPluginImpl()) {}

void SignerManagerPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("SignerManagerPlugin Initialize");
  impl->initialize();
}

void SignerManagerPlugin::setProgramOptions(options_description &cfg) {}

void SignerManagerPlugin::pluginStart() {
  logger::INFO("SignerManagerPlugin Start");
}

SignerManagerPlugin::~SignerManagerPlugin() {
  impl.reset();
}
} // namespace gruut
