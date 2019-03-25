#pragma once

#include "../../../include/json.hpp"
#include "../../../src/plugins/net_plugin/config/include/message.hpp"
#include "channel.hpp"
#include <vector>

using namespace gruut::net_plugin;
using namespace std;

// TODO: need to separate data structure and channel.
struct InNetMsg {
  MessageType type;
  nlohmann::json body;
  id_type sender_id;
};

namespace appbase::incoming {
  namespace channels {
    using network = ChannelTypeTemplate<InNetMsg>;
  };
}
