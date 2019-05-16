#include "include/block_producer_plugin.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../chain_plugin/include/chain_plugin.hpp"
#include <boost/asio/steady_timer.hpp>

namespace gruut {

using namespace std;

class BlockProducerPluginImpl {
public:
  void initialize() {
    timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
  }

  void start() {
    scheduleBlockProductionLoop();
  }

private:
  void scheduleBlockProductionLoop() {
    auto wake_up_time = calculateNextBlockTime();
    timer->expires_from_now(wake_up_time);
    timer->async_wait([this](boost::system::error_code ec) {
      if (!ec) {
        // produce a block.
        scheduleBlockProductionLoop();
      } else {
        logger::ERROR("Error from block producer timer: {}", ec.message());
      }
    });
  }

  std::chrono::seconds calculateNextBlockTime() {
    std::chrono::seconds minimum_delay_time(5);

    // TODO: 일정 개수 이전의 블록들을 기준으로 계산합니다. 현재는 임시로 상수값
    float d = 0.5;
    float b = 0.1;
    float signers_count = 10;

    // calculate the distance between a node and an optimal merger.
    auto mergers_distance = calculateDistanceBetweenMergers();
    // calculate the distance between signers and an optimal signer.
    auto signers_distance = calculateDistanceBetweenSigners();

    // TODO: temporary value
    return minimum_delay_time;
  }

  float calculateDistanceBetweenMergers() {
    // TODO: a number of producers.
    const int producersCount = 10;
    auto& chain = dynamic_cast<ChainPlugin*>(app().getPlugin("ChainPlugin"))->chain();

    return 0.0;
  }


  float calculateDistanceBetweenSigners() {

  }

  unique_ptr<boost::asio::steady_timer> timer;
};

void BlockProducerPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("BlockProducerPlugin Initialize");

  impl->initialize();
}

void BlockProducerPlugin::pluginStart() {
  logger::INFO("BlockProducerPlugin Start");

  impl->start();
}

BlockProducerPlugin::BlockProducerPlugin() : impl(make_unique<BlockProducerPluginImpl>()) {}

BlockProducerPlugin::~BlockProducerPlugin() {
  impl.reset();
}
} // namespace gruut
