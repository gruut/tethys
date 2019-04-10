#pragma once

#include "../../../../lib/appbase/include/channel.hpp"
#include "../../../include/json.hpp"
#include "../../net_plugin/config/include/message.hpp"
#include <vector>

using namespace gruut::net_plugin;
using namespace std;

namespace appbase::incoming {
namespace channels {
using network = ChannelTypeTemplate<struct network_tag, InNetMsg>;
using transaction = ChannelTypeTemplate<struct transaction_tag, nlohmann::json>;
}; // namespace channels
} // namespace appbase::incoming

namespace appbase::outgoing {
namespace channels {
using network = ChannelTypeTemplate<struct network_tag, OutNetMsg>;
};
} // namespace appbase::outgoing
