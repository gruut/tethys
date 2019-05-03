#include "include/chain.hpp"
#include "../../../lib/log/include/log.hpp"
#include <utility>

namespace gruut {

using namespace nlohmann;

struct LocalChainState {
  // chain
  string chain_id;
  string world_id;
  string chain_created_time;

  // policy
  bool allow_custom_contract;
  bool allow_oracle;
  bool allow_tag;
  bool allow_heavy_contract;

  // creator
  string creator_id;
  vector<string> creator_pk;
  string creator_sig;
};

struct GenesisState {
  // world
  string world_id;
  string world_created_time;

  // key_currency
  string keyc_name;
  string initial_amount;

  // mining_policy
  bool allow_mining;
  void *rule = nullptr;

  // user_policy
  bool allow_anonymous_user;
  string join_fee;

  LocalChainState local_chain_state;

  // authority
  string authority_id;
  vector<string> authority_cert;

  // creator
  string creator_id;
  vector<string> cert;
  string sig;
};

class ChainImpl {
public:
  ChainImpl(Chain &self) : self(self) {}

  void init(json genesis_state) {
    GenesisState genesis = unmarshalGenesisState(genesis_state);
  }

  GenesisState unmarshalGenesisState(json state) {
    try {
      GenesisState genesis_state;

      genesis_state.world_id = state["/world/id"_json_pointer];
      genesis_state.world_created_time = state["/world/after"_json_pointer];

      genesis_state.keyc_name = state["/key_currency/name"_json_pointer];
      genesis_state.initial_amount = state["/key_currency/initial_amount"_json_pointer];

      genesis_state.allow_mining = state["/mining_policy/allow_mining"_json_pointer];

      genesis_state.allow_anonymous_user = state["/user_policy/allow_anonymous_user"_json_pointer];
      genesis_state.join_fee = state["/user_policy/join_fee"_json_pointer];

      genesis_state.local_chain_state.chain_id = state["/eden/chain/id"_json_pointer];
      genesis_state.local_chain_state.world_id = state["/eden/chain/world"_json_pointer];
      genesis_state.local_chain_state.chain_created_time = state["/eden/chain/after"_json_pointer];

      genesis_state.local_chain_state.allow_custom_contract = state["/eden/policy/allow_custom_contract"_json_pointer];
      genesis_state.local_chain_state.allow_oracle = state["/eden/policy/allow_oracle"_json_pointer];
      genesis_state.local_chain_state.allow_tag = state["/eden/policy/allow_tag"_json_pointer];
      genesis_state.local_chain_state.allow_heavy_contract = state["/eden/policy/allow_heavy_contract"_json_pointer];

      genesis_state.local_chain_state.creator_id = state["/eden/creator/id"_json_pointer];
      genesis_state.local_chain_state.creator_pk = state["eden"]["creator"]["pk"].get<vector<string>>();
      genesis_state.local_chain_state.creator_sig = state["/eden/creator/sig"_json_pointer];

      genesis_state.authority_id = state["/authority/id"_json_pointer];
      genesis_state.authority_cert = state["authority"]["cert"].get<vector<string>>();

      genesis_state.creator_id = state["/creator/id"_json_pointer];
      genesis_state.cert = state["creator"]["cert"].get<vector<string>>();
      genesis_state.sig = state["/creator/sig"_json_pointer];

      return genesis_state;
    } catch (json::parse_error &e) {
      logger::ERROR("Failed to parse world_create.json: {}", e.what());
      throw e;
    }
  }

  Chain &self;
};

Chain::Chain() {
  impl = make_unique<ChainImpl>(*this);
}

void Chain::startup(nlohmann::json &genesis_state) {
  impl->init(genesis_state);
}

Chain::~Chain() {
  impl.reset();
}
} // namespace gruut
