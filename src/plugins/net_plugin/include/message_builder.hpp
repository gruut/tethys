#pragma once

#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "msg_schema.hpp"

#include <optional>
#include <type_traits>
#include <variant>

using namespace grpc;
using namespace std;

namespace gruut {
namespace net_plugin {

class MessageBuilder {
public:
  template <MessageType T, typename = enable_if_t<T == MessageType::MSG_LEAVE>>
  static auto build(string from) {
    InNetMsg incoming_msg;

    incoming_msg.type = MessageType::MSG_LEAVE;
    nlohmann::json msg_body;

    msg_body["sID"] = from;
    msg_body["time"] = TimeUtil::now();
    msg_body["msg"] = "disconnected with signer";
    sender_id_type to;
    copy(from.begin(), from.end(), to.begin());

    incoming_msg.body = msg_body;
    incoming_msg.sender_id = to;

    return incoming_msg;
  }
};

} // namespace net_plugin
} // namespace gruut