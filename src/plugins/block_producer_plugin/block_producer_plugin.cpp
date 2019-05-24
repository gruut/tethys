#include "include/block_producer_plugin.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../chain_plugin/include/chain_plugin.hpp"
#include <boost/asio/steady_timer.hpp>
#include <chrono>

namespace gruut {

using namespace std;

// TODO: temporary setting up fixed numbers of producers.
constexpr int PRODUCERS_COUNT = 10;

constexpr int SIGNERS_SIZE = 100;

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
    float minimum_delay_time(5);

    // TODO: 일정 개수 이전의 블록들을 기준으로 계산합니다. 현재는 임시로 상수값
    float d = 0.5;
    float b = 0.1;
    float signers_count = 10;

    vector<Block> previousBlocksOrderByDesc = move(getPreviousBlocks());

    // calculate the distance between a node and an optimal merger.
    auto mergers_distance = calculateDistanceBetweenMergers(previousBlocksOrderByDesc);
    // calculate the distance between signers and an optimal signer.
    auto signers_distance = calculateDistanceBetweenSigners(previousBlocksOrderByDesc);

    // clang-format off
    auto delay_seconds = std::chrono::duration<float>(minimum_delay_time + (d + (b * (exp(mergers_distance) - 1)) * (1 + signers_distance)));
    // clang-format on

    return std::chrono::duration_cast<std::chrono::seconds>(delay_seconds);
  }

  vector<Block> getPreviousBlocks() {
    auto& chain = dynamic_cast<ChainPlugin*>(app().getPlugin("ChainPlugin"))->chain();
    auto latestHeight = chain.getLatestResolvedHeight();
    int from = latestHeight - PRODUCERS_COUNT + 1;
    if (from <= 0)
      from = 1;

    vector<Block> blocks = chain.getBlocksByHeight(from, latestHeight);
    sort(blocks.begin(), blocks.end(), [](auto &left, auto &right) {
        return left.getHeight() > right.getHeight();
    });

    return blocks;
  }

  float calculateDistanceBetweenMergers(vector<Block> &blocks) {
    int previousProducerCount = PRODUCERS_COUNT;
    if (blocks.size() != PRODUCERS_COUNT) {
      previousProducerCount = blocks.size();
    }

    bitset<256> id_bits = getOptimalMergerId(blocks, previousProducerCount);

    auto my_id = idToBitSet(app().getId());
    float dist = getHammingDistance(id_bits, my_id);

    return dist;
  }

  bitset<256> getOptimalMergerId(vector<Block> blocks, const int producers_count) {
    using byte = bitset<8>;
    using producer_bitset = vector<byte>;

    bitset<256> optimal_merger_id;

    vector<producer_bitset> block_producer_ids;
    block_producer_ids.reserve(producers_count);

    for (int producer_index = 0; producer_index < producers_count; ++producer_index) {
      base58_type block_prod_id = blocks.at(producer_index).getBlockProdId();
      string raw_prod_id = TypeConverter::decodeBase<58>(block_prod_id);

      vector<bitset<8>> bytes;
      for (auto i = 0; i < raw_prod_id.size(); ++i) {
        bytes.emplace_back(byte(raw_prod_id[i]));
      }

      block_producer_ids.push_back(bytes);
    }

    for (int byte_position = 0; byte_position < 32; ++byte_position) {
      for (int bit_position = 0; bit_position < 8; ++bit_position) {
        double count_0 = 0;
        double count_1 = 0;

        for (int producer_index = 0; producer_index < producers_count; ++producer_index) {
          auto bit(block_producer_ids.at(producer_index)[byte_position][bit_position]);
          if (bit == 0) {
            count_0 += pow(1.414, static_cast<double>(producers_count - bit_position));
          } else {
            count_1 += pow(1.414, static_cast<double>(producers_count - bit_position));
          }

          optimal_merger_id[bit_position] = count_0 > count_1 ? 1 : 0;
        }
      }
    }

    return optimal_merger_id;
  }

  float calculateDistanceBetweenSigners(vector<Block> &blocks) {
    assert(blocks.size() > 0);

    bitset<256> optimal_signer_id_bits = getOptimalSignerId(blocks[0]);

    const int signers_size = SIGNERS_SIZE;
    auto signer_pool_manager_ptr = dynamic_cast<NetPlugin*>(app().getPlugin("NetPlugin"))->getSignerPoolManager();
    vector<Signer> signers = signer_pool_manager_ptr->getSigners(signers_size);

    vector<bitset<256>> signer_ids_bits;
    transform(signers.begin(), signers.end(), back_inserter(signer_ids_bits), [this](Signer &signer){
      return idToBitSet(signer.user_id);
    });

    float dist = 0;
    for (auto &signer_id_bits : signer_ids_bits) {
      for (auto i = 0; i < 4; ++i) {
        bitset<64> bits;
        bitset<64> partial_optimal_id_bits;

        for (auto j = 0; j < 64; ++j) {
          bits[j] = signer_id_bits[i * j];
          partial_optimal_id_bits[j] =  optimal_signer_id_bits[i * j];
        }

        dist += exponentialDistance((getHammingDistance(bits, partial_optimal_id_bits) % 11));
      }
    }

    return dist;
  }

  float exponentialDistance(float dist) {
    return exp(dist) - 1;
  }

  bitset<256> getOptimalSignerId(Block &latest_block) {
    auto tx_root = latest_block.getTxRoot();
    auto us_root = latest_block.getUserStateRoot();
    auto cs_root = latest_block.getContractStateRoot();
    auto sg_root = latest_block.getSgRoot();

    hash_t optimal_signer_id = Sha256::hash(tx_root + us_root + cs_root + sg_root);

    string id(optimal_signer_id.begin(), optimal_signer_id.end());
    return idToBitSet(id);
  }

  bitset<256> idToBitSet(string_view id) {
    string bits_str;

    for (int i = 0; i < id.size(); ++i) {
      bits_str += bitset<8>(id[i]).to_string();
    }

    return bitset<256>(bits_str);
  }

  template<typename T>
  int getHammingDistance(T &optimal_id, T &my_id) {
    auto result = optimal_id ^ my_id;

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
