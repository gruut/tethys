#include "include/chain.hpp"
#include "../../../lib/appbase/include/application.hpp"

namespace tethys {

class ChainImpl {
public:
  ChainImpl(Chain &self) : self(self) {}

  void initWorld(nlohmann::json &world_state) {
    world_type world = unmarshalWorldState(world_state);

    appbase::app().setWorldId(world.world_id);

    self.saveWorld(world);
  }

  optional<vector<string>> initChain(nlohmann::json &chain_state) {
    local_chain_type chain = unmarshalChainState(chain_state);

    if (appbase::app().getWorldId() != chain.world_id)
      return {};

    appbase::app().setChainId(chain.chain_id);

    self.saveChain(chain);

    return chain.tracker_addresses;
  }

  world_type unmarshalWorldState(nlohmann::json &state) {
    try {
      world_type world_state;

      world_state.world_id = state["/world/id"_json_pointer];
      world_state.world_created_time = state["/world/after"_json_pointer];

      world_state.keyc_name = state["/key_currency/name"_json_pointer];
      world_state.initial_amount = state["/key_currency/initial_amount"_json_pointer];

      world_state.allow_mining = state["/mining_policy/allow_mining"_json_pointer];

      world_state.allow_anonymous_user = state["/user_policy/allow_anonymous_user"_json_pointer];
      world_state.join_fee = state["/user_policy/join_fee"_json_pointer];

      world_state.eden_sig = state["eden_sig"].get<string>();

      world_state.authority_id = state["/authority/id"_json_pointer];
      world_state.authority_cert = state["authority"]["cert"].get<vector<string>>();

      world_state.creator_id = state["/creator/id"_json_pointer];
      world_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      world_state.creator_sig = state["/creator/sig"_json_pointer];

      return world_state;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse world json: {}", e.what());
      throw e;
    }
  }

  local_chain_type unmarshalChainState(nlohmann::json &state) {
    try {
      local_chain_type chain_state;

      chain_state.chain_id = state["/chain/id"_json_pointer];
      chain_state.world_id = state["/chain/world"_json_pointer];
      chain_state.chain_created_time = state["/chain/after"_json_pointer];

      chain_state.allow_custom_contract = state["/policy/allow_custom_contract"_json_pointer];
      chain_state.allow_oracle = state["/policy/allow_oracle"_json_pointer];
      chain_state.allow_tag = state["/policy/allow_tag"_json_pointer];
      chain_state.allow_heavy_contract = state["/policy/allow_heavy_contract"_json_pointer];

      chain_state.creator_id = state["/creator/id"_json_pointer];
      chain_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      chain_state.creator_sig = state["/creator/sig"_json_pointer];

      for (auto &tracker_info : state["tracker"]) {
        auto address = tracker_info["address"].get<string>();
        chain_state.tracker_addresses.emplace_back(address);
      }

      return chain_state;
    } catch (nlohmann::json::parse_error &e) {
      logger::ERROR("Failed to parse local chain json: {}", e.what());
      throw e;
    }
  }

  Chain &self;
};

void Chain::initWorld(nlohmann::json &world_state) {
  impl->initWorld(world_state);
}

optional<vector<string>> Chain::initChain(nlohmann::json &chain_state) {
  return impl->initChain(chain_state);
}

Chain::Chain(string_view dbms, string_view table_name, string_view db_user_id, string_view db_password) {
  impl = make_unique<ChainImpl>(*this);
  rdb_controller = make_unique<RdbController>(dbms, table_name, db_user_id, db_password);
  kv_controller = make_unique<KvController>();
}

Chain::~Chain() {
  impl.reset();
}

// RDB functions
void Chain::insertBlockData(tethys::Block &block_info) {
  rdb_controller->insertBlockData(block_info);
}

void Chain::insertTransactionData(tethys::Block &block_info) {
  rdb_controller->insertTransactionData(block_info);
}

vector<Block> Chain::getBlocksByHeight(int from, int to) {
  if (from > to) {
    return vector<Block>();
  }

  stringstream ss;
  ss << "block_height BETWEEN " << from << " AND " << to;

  vector<Block> blocks = rdb_controller->getBlocks(ss.str());
  return blocks;
}

block_height_type Chain::getLatestResolvedHeight() {
  string condition = "ORDER BY block_height DESC LIMIT 1";
  Block block = rdb_controller->getBlock(condition);

  return block.getHeight();
}

string Chain::getUserCert(const base58_type &user_id) {
  return rdb_controller->getUserCert(user_id);
}

// KV functions
void Chain::saveWorld(world_type &world_info) {
  kv_controller->saveWorld(world_info);
}

void Chain::saveChain(local_chain_type &chain_info) {
  kv_controller->saveChain(chain_info);
}

void Chain::saveBackup(UnresolvedBlock &block_info) {
  kv_controller->saveBackup(block_info);
}

void Chain::saveSelfInfo(self_info_type &self_info) {
  kv_controller->saveSelfInfo(self_info);
}

string Chain::getValueByKey(string what, const string &base_keys) {
  return kv_controller->getValueByKey(what, base_keys);
}

} // namespace tethys
