#include "include/chain.hpp"

namespace gruut {

class ChainImpl {
public:
  ChainImpl(Chain &self) : self(self) {}

  void init(nlohmann::json genesis_state) {
    world_type genesis = unmarshalGenesisState(genesis_state);

    // genesis가 여기 있으므로, world에 관한 정보를 KV를 넣는 코드가 여기에 있어야 할 것이다.
  }

  world_type unmarshalGenesisState(nlohmann::json state) {
    try {
      world_type genesis_state;

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

      genesis_state.authority_id = state["/authority/id"_json_pointer];
      genesis_state.authority_cert = state["authority"]["cert"].get<vector<string>>();

      genesis_state.creator_id = state["/creator/id"_json_pointer];
      genesis_state.local_chain_state.creator_id = state["/creator/id"_json_pointer];
      genesis_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      genesis_state.local_chain_state.creator_cert = state["creator"]["cert"].get<vector<string>>();
      genesis_state.creator_sig = state["/creator/sig"_json_pointer];
      genesis_state.local_chain_state.creator_sig = state["/creator/sig"_json_pointer];

      assert(genesis_state.world_id == genesis_state.local_chain_state.world_id);
      assert(genesis_state.creator_id == genesis_state.local_chain_state.creator_id);
      assert(genesis_state.creator_cert == genesis_state.local_chain_state.creator_cert);
      assert(genesis_state.creator_sig == genesis_state.local_chain_state.creator_sig);

      return genesis_state;
    } catch (nlohmann::json::parse_error &e) {
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
