#pragma once

#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "msg_schema.hpp"

#include <optional>

#include <optional>
using namespace grpc;
using namespace std;

namespace gruut {
namespace net_plugin {

class MessageHandler {
public:
  optional<InNetMsg> genInternalMsg(MessageType msg_type, b58_id_type &b58_id) {
    nlohmann::json msg_body;

    switch (msg_type) {
    case MessageType::MSG_LEAVE: {
      msg_body["sID"] = b58_id;
      msg_body["time"] = TimeUtil::now();
      msg_body["msg"] = "disconnected with signer";

      return InNetMsg{msg_body, msg_type, b58_id};
    }

    default:
      return {};
    }
  }
};
} // namespace net_plugin
} // namespace gruut