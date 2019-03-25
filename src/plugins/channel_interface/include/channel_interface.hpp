#pragma once

#include "../../../include/json.hpp"
#include "../../net_plugin/config/include/message.hpp"
#include "../../../../lib/appbase/include/channel.hpp"
#include <vector>

using namespace gruut::net_plugin;
using namespace std;

// TODO: need to separate data structure and channel.
struct InNetMsg {
  MessageType type;
  nlohmann::json body;
  id_type sender_id;
};

struct OutNetMsg {
  MessageType type;
  nlohmann::json body;
  vector<id_type> receivers;
};

namespace appbase::incoming {
  namespace channels {
    using network = ChannelTypeTemplate<InNetMsg>;
  };
}

namespace appbase::outgoing {
  namespace channels {
    using network = ChannelTypeTemplate<OutNetMsg>;
  };
}
