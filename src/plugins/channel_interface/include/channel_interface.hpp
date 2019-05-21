#pragma once

#include "../../../../lib/appbase/include/channel.hpp"
#include "../../../include/json.hpp"
#include "../../chain_plugin/structure/transaction.hpp"
#include "../../net_plugin/config/include/message.hpp"
#include <vector>

using namespace gruut::net_plugin;
using namespace std;

namespace appbase::incoming {
namespace channels {
using net_control = ChannelTypeTemplate<struct net_control_tag, nlohmann::json>;
using network = ChannelTypeTemplate<struct network_tag, InNetMsg>;
using transaction = ChannelTypeTemplate<struct transaction_tag, nlohmann::json>;
using block = ChannelTypeTemplate<struct block_tag, nlohmann::json>;

using transaction_pool = ChannelTypeTemplate<struct tx_pool_tag, vector<gruut::Transaction>>;
}; // namespace channels
} // namespace appbase::incoming

namespace appbase::outgoing {
namespace channels {
using network = ChannelTypeTemplate<struct network_tag, OutNetMsg>;
};
} // namespace appbase::outgoing
