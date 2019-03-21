#pragma once

#include <vector>
#include "channel.hpp"
#include "../../../include/json.hpp"
#include "../../../src/plugins/net_plugin/config/include/message.hpp"

using namespace gruut::net_plugin;
using namespace std;

struct TempData {

};

struct InNetMsg {
  MessageType type;
  nlohmann::json body;
  id_type sender_id;
};

struct OutNetMsg {
  MessageType type;
  nlohmann::json body;
  vector<id_type> receiver_ids;
};

namespace appbase {
  namespace channels {
    using temp_channel = ChannelTypeTemplate<TempData>;
    using in_msg_channel = ChannelTypeTemplate<InNetMsg>;
    using out_msg_channel = ChannelTypeTemplate<OutNetMsg>;
  }
}