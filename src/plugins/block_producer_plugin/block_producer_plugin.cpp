#include "include/block_producer_plugin.hpp"
#include "../../../lib/appbase/include/application.hpp"
#include "../chain_plugin/include/chain_plugin.hpp"
#include "include/ssig_pool.hpp"
#include <algorithm>
#include <array>
#include <boost/asio/steady_timer.hpp>
#include <chrono>

namespace tethys {

using namespace std;

// TODO: temporary setting up fixed numbers of producers.
constexpr int PRODUCERS_COUNT = 10;

constexpr int SIGNERS_SIZE = 100;

struct PartialBlock {
  base58_type id;
  timestamp_t time;
  alphanumeric_type world_id;
  alphanumeric_type chain_id;
  block_height_type height;
  base58_type prev_id;
  base64_type txroot;
  base64_type usroot;
  base64_type csroot;
  base64_type link;

  vector<string> txagg_cbor_list;
};

class BlockProducerPluginImpl {
public:
  incoming::channels::ssig::channel_type::Handle ssig_channel_subscription;

  void initialize() {
    timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
    ssig_pool = make_unique<SupportSigPool>();
  }

  void start() {
    scheduleBlockProductionLoop();
  }

  void pushSupportSig(nlohmann::json &msg_ssig) {
    auto info = msg_ssig["singer"];
    auto signer_id = json::get<string>(info, "id").value();

    auto found = find_if(selected_signers.begin(), selected_signers.end(),
                         [&signer_id](auto &signer_info) { return signer_info.first.user_id == signer_id; });

    if (found == selected_signers.end()) {
      logger::ERROR("[BP] Unrequested supporter : {}", signer_id);
      return;
    }

    auto ssig = json::get<string>(info, "sig").value();
    auto signer_cert = found->first.pk;

    // TODO : verify support sig using MSG_REQ_SSIG info & ssig

    SupportSigInfo ssig_info;
    ssig_info.id = signer_id;
    ssig_info.sig = ssig;
    ssig_info.cert = signer_cert;

    ssig_pool->add(ssig_info);
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
    auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();

    auto latestHeight = chain.getLatestResolvedHeight();
    int from = latestHeight - PRODUCERS_COUNT + 1;
    if (from <= 0)
      from = 1;

    vector<Block> blocks = chain.getBlocksByHeight(from, latestHeight);
    sort(blocks.begin(), blocks.end(), [](auto &left, auto &right) { return left.getHeight() > right.getHeight(); });

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
    auto user_pool_manager_ptr = dynamic_cast<NetPlugin *>(app().getPlugin("NetPlugin"))->getUserPoolManager();

    selected_signers = user_pool_manager_ptr->getCloseSigners(optimal_signer_id_bits, signers_size);

    float dist = 0;
    for (auto &[_, signer_id_bits] : selected_signers) {
      for (auto i = 0; i < 4; ++i) {
        bitset<64> bits;
        bitset<64> partial_optimal_id_bits;

        for (auto j = 0; j < 64; ++j) {
          bits[j] = signer_id_bits[i * j];
          partial_optimal_id_bits[j] = optimal_signer_id_bits[i * j];
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

  template <typename T>
  int getHammingDistance(T &optimal_id, T &my_id) {
    auto result = optimal_id ^ my_id;

    return result.count();
  }

  PartialBlock makePartialBlock() {
    auto &chain = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->chain();

    // TODO : get latest block info from unresolved long chain
    UnresolvedBlock ur_latest_block;

    // TODO : exception handling (if unresolved pool is empty )

    auto block_time = TimeUtil::nowBigInt();
    auto world_id = app().getWorldId();
    auto chain_id = app().getChainId();
    auto height = ur_latest_block.block.getHeight() + 1;
    auto block_prev_id = ur_latest_block.block.getBlockId();

    auto vec_link = Sha256::hash(ur_latest_block.block.getBlockProdSig());
    auto link = TypeConverter::encodeBase<64>(vec_link);

    int latest_resolved_block_height = height - BLOCK_CONFIRM_LEVEL;
    base64_type csroot, usroot;
    if (latest_resolved_block_height > 0) {
      auto r_latest_block = chain.getBlocksByHeight(latest_resolved_block_height, latest_resolved_block_height);
      csroot = r_latest_block[0].getContractStateRoot();
      usroot = r_latest_block[0].getUserStateRoot();
    } else {
      array<uint8_t, 32> empty_bytes{0};
      base64_type b64_empty_data = TypeConverter::encodeBase<64>(empty_bytes); // AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=
      csroot = b64_empty_data;
      usroot = b64_empty_data;
    }

    BytesBuilder builder;
    builder.append(app().getId());
    builder.appendDec(block_time);
    builder.append(world_id);
    builder.append(chain_id);
    builder.appendDec(height);
    builder.appendBase<58>(block_prev_id);
    builder.appendBase<64>(link);

    auto raw_block_id = builder.getBytes();
    auto hashed_block_id = Sha256::hash(raw_block_id);
    auto block_id = TypeConverter::encodeBase<58>(hashed_block_id);

    PartialBlock partial_block;

    partial_block.id = block_id;
    partial_block.time = block_time;
    partial_block.world_id = world_id;
    partial_block.chain_id = chain_id;
    partial_block.height = height;
    partial_block.prev_id = block_prev_id;
    partial_block.link = link;
    partial_block.usroot = usroot;
    partial_block.csroot = csroot;

    auto &tx_pool = dynamic_cast<ChainPlugin *>(app().getPlugin("ChainPlugin"))->transactionPool();
    auto transactions = tx_pool.fetchAll();

    vector<hash_t> hashed_txagg_cbor_list;

    for (auto &tx : transactions) {
      auto txagg_cbor = tx.getTxAggCbor();
      partial_block.txagg_cbor_list.push_back(txagg_cbor);
      hashed_txagg_cbor_list.push_back(Sha256::hash(txagg_cbor));
    }

    StaticMerkleTree merkle_tree;
    merkle_tree.generate(hashed_txagg_cbor_list);
    partial_block.txroot = TypeConverter::encodeBase<64>(merkle_tree.getStaticMerkleTree().at(0));

    return partial_block;
  }

  nlohmann::json makeBaseMsgBlock(PartialBlock &partial_block) {
    nlohmann::json block;
    block["id"] = partial_block.id;
    block["time"] = to_string(partial_block.time);
    block["world"] = partial_block.world_id;
    block["chain"] = partial_block.chain_id;
    block["height"] = to_string(partial_block.height);
    block["previd"] = partial_block.prev_id;
    block["link"] = partial_block.link;

    return block;
  }

  OutNetMsg makeMsgReqSsig(PartialBlock &partial_block) {
    nlohmann::json block = makeBaseMsgBlock(partial_block);
    block["txroot"] = partial_block.txroot;
    block["usroot"] = partial_block.usroot;
    block["csroot"] = partial_block.csroot;

    nlohmann::json producer;
    producer["id"] = TypeConverter::encodeBase<58>(app().getId());
    producer["sig"] = signMsgReqSsig(partial_block.id, partial_block.txroot, partial_block.usroot, partial_block.csroot);

    nlohmann::json req_ssig_body;
    req_ssig_body["block"] = block;
    req_ssig_body["producer"] = producer;

    OutNetMsg msg_req_ssig;
    msg_req_ssig.type = MessageType::MSG_REQ_SSIG;
    msg_req_ssig.body = req_ssig_body;

    for (auto &[signer, _] : selected_signers)
      msg_req_ssig.receivers.push_back(signer.user_id);

    return msg_req_ssig;
  }

  string signMsgReqSsig(const string &block_id, const string &txroot, const string &usroot, const string &csroot) {
    BytesBuilder builder;
    builder.appendBase<58>(block_id);
    builder.appendBase<64>(txroot);
    builder.appendBase<64>(usroot);
    builder.appendBase<64>(csroot);

    auto sig = ECDSA::doSign(app().getSk(), builder.getBytes(), app().getPass());

    return TypeConverter::encodeBase<64>(sig);
  }

  OutNetMsg makeMsgBlock(PartialBlock &partial_block) {
    nlohmann::json block = makeBaseMsgBlock(partial_block);

    BytesBuilder bytes_builder;
    bytes_builder.appendBase<58>(partial_block.id);
    bytes_builder.appendBase<64>(partial_block.txroot);
    bytes_builder.appendBase<64>(partial_block.usroot);
    bytes_builder.appendBase<64>(partial_block.csroot);
    auto hash_data = Sha256::hash(bytes_builder.getBytes());
    auto b64_hash = TypeConverter::encodeBase<64>(hash_data);
    block["hash"] = b64_hash;

    nlohmann::json state, signer, certificate;

    nlohmann::json my_cert_info;
    auto b58_my_id = TypeConverter::encodeBase<58>(app().getId());
    my_cert_info["id"] = b58_my_id;
    my_cert_info["cert"] = app().getMyCert();
    certificate.push_back(my_cert_info);

    auto ssig_list = ssig_pool->fetchAll();
    vector<hash_t> hashed_ssig_list;

    for (auto &ssig : ssig_list) {
      nlohmann::json ssig_info;
      ssig_info["id"] = ssig.id;
      ssig_info["sig"] = ssig.sig;
      signer.push_back(ssig_info);

      nlohmann::json cert_info;
      cert_info["id"] = ssig.id;
      cert_info["cert"] = ssig.cert;
      certificate.push_back(cert_info);

      bytes_builder.clear();
      bytes_builder.appendBase<58>(ssig.id);
      bytes_builder.appendBase<64>(ssig.sig);
      auto hashed_data = Sha256::hash(bytes_builder.getBytes());
      hashed_ssig_list.push_back(hashed_data);
    }

    state["txroot"] = partial_block.txroot;
    state["usroot"] = partial_block.usroot;
    state["csroot"] = partial_block.csroot;
    StaticMerkleTree merkle_tree;
    merkle_tree.generate(hashed_ssig_list);
    auto sgroot = TypeConverter::encodeBase<64>(merkle_tree.getStaticMerkleTree().at(0));
    state["sgroot"] = sgroot;

    nlohmann::json tx(partial_block.txagg_cbor_list);

    nlohmann::json producer;
    producer["id"] = b58_my_id;
    auto tx_len = static_cast<int>(tx.size());
    auto signer_len = static_cast<int>(signer.size());
    auto cur_time = TimeUtil::nowBigInt();
    producer["sig"] = signMsgBlock(cur_time, b64_hash, tx_len, signer_len, sgroot);

    nlohmann::json msg_block_body;
    msg_block_body["btime"] = to_string(cur_time);
    msg_block_body["block"] = block;
    msg_block_body["tx"] = tx;
    msg_block_body["state"] = state;
    msg_block_body["signer"] = signer;
    msg_block_body["certificate"] = certificate;
    msg_block_body["producer"] = producer;

    OutNetMsg msg_block;
    msg_block.type = MessageType::MSG_BLOCK;
    msg_block.body = msg_block_body;

    return msg_block;
  }

  string signMsgBlock(uint64_t btime, const string &hash, int tx_len, int signer_len, const string &sgroot) {
    BytesBuilder builder;
    builder.appendDec(btime);
    builder.appendBase<64>(hash);
    builder.appendDec(tx_len);
    builder.appendDec(signer_len);
    builder.appendBase<64>(sgroot);

    auto sig = ECDSA::doSign(app().getSk(), builder.getBytes(), app().getPass());

    return TypeConverter::encodeBase<64>(sig);
  }

  unique_ptr<SupportSigPool> ssig_pool;
  unique_ptr<boost::asio::steady_timer> timer;
  vector<pair<User, bitset<256>>> selected_signers;
};

void BlockProducerPlugin::pluginInitialize(const boost::program_options::variables_map &options) {
  logger::INFO("BlockProducerPlugin Initialize");

  auto &ssig_channel = app().getChannel<incoming::channels::ssig>();
  impl->ssig_channel_subscription = ssig_channel.subscribe([this](auto data) { impl->pushSupportSig(data); });

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
} // namespace tethys
