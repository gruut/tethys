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

    // TODO: Optimal Merger를 계산하는데 최신 생성자의 ID만 있으면 되므로, from은 필요가 없음
    // 논의 후 제거
    // https://thevaulters.atlassian.net/wiki/spaces/SGN/pages/90832930/PoP+in+Public+Network
    //    int from = latestHeight - producersCount + 1;
    //    if (from <= 0)
    //      from = 1;

    vector<Block> blocks = chain.getBlocksByHeight(latestHeight, latestHeight);

    if (blocks.size() != producersCount) {
      producersCount = blocks.size();
    }

    base58_type opitmal_merger_id = getOptimalMergerId(blocks.at(0), producersCount);

    return 0.0;
  }

  base58_type getOptimalMergerId(Block latest_block, const int producersCount) {
    base58_type block_prod_id = latest_block.getBlockProdId();
    vector<uint8_t> block_prod_id_bytes = TypeConverter::stringToBytes(TypeConverter::decodeBase<58>(block_prod_id));

    vector<uint8_t> optimal_merger_id_bytes;

    for_each(block_prod_id_bytes.begin(), block_prod_id_bytes.end(), [&](uint8_t val){
      bitset<8> byte(val);

      bitset<8> flipped_byte = byte.flip();
      optimal_merger_id_bytes.push_back(flipped_byte.to_ulong());
    });

    return TypeConverter::encodeBase<58>(optimal_merger_id_bytes);
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
