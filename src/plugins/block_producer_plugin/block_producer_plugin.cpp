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
    // TODO: fixed numbers of producers.
    int producersCount = 10;
    auto& chain = dynamic_cast<ChainPlugin*>(app().getPlugin("ChainPlugin"))->chain();

    auto latestHeight = chain.getLatestResolvedHeight();

    int from = latestHeight - producersCount + 1;
    if (from <= 0)
      from = 1;

    vector<Block> blocks = chain.getBlocksByHeight(from, latestHeight);
    sort(blocks.begin(), blocks.end(), [](auto &left, auto &right) {
        return left.getHeight() > right.getHeight();
    });

    if (blocks.size() != producersCount) {
      producersCount = blocks.size();
    }

    bitset<256> id_bits = getOptimalMergerId(blocks, producersCount);

    bitset<256> my_id(app().getId());
    int dist = getHammingDistance(id_bits, my_id);

    return 0.0;
  }

  bitset<256> getOptimalMergerId(vector<Block> blocks, const int producers_count) {
    bitset<256> optimal_merger_id;

    vector<bitset<256>> block_producer_ids;

    for (int producer_index = 0; producer_index < producers_count; ++producer_index) {
      base58_type block_prod_id = blocks.at(producer_index).getBlockProdId();

      block_producer_ids.emplace_back(bitset<256>(TypeConverter::decodeBase<58>(block_prod_id)));
    }

    for (int bit_position = 0; bit_position < 256; ++bit_position) {
      double count_0 = 0;
      double count_1 = 0;

      for (int producer_index = 0; producer_index < producers_count; ++producer_index) {
        auto bit(block_producer_ids.at(producer_index)[bit_position]);
        if (bit == 0) {
          count_0 += pow(1.414, static_cast<double>(producers_count - bit_position));
        } else {
          count_1 += pow(1.414, static_cast<double>(producers_count - bit_position));
        }

        optimal_merger_id[bit_position] = count_0 > count_1 ? 1 : 0;
      }
    }

    return optimal_merger_id;
  }

  float calculateDistanceBetweenSigners() {

  }

  int getHammingDistance(bitset<256> &optimal_merger_id, bitset<256> &my_id) {
    auto result = optimal_merger_id ^ my_id;

    return result.count();
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
