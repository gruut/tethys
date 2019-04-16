#pragma once

#include "../../../../lib/gruut-utils/src/hmac_key_maker.hpp"
#include "../../../../lib/gruut-utils/src/time_util.hpp"
#include "../../../../lib/json/include/json.hpp"
#include "../../channel_interface/include/channel_interface.hpp"
#include "../config/include/network_config.hpp"
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
  template <MessageType T, typename = enable_if_t<T == MessageType::MSG_CHALLENGE>>
  static auto build(const string &b58_recv_id, const string &merger_nonce) {
    OutNetMsg msg_challenge;
    nlohmann::json msg_body;

    msg_body["time"] = TimeUtil::now();
    msg_body["user"] = b58_recv_id;
    msg_body["merger"] = MY_ID_BASE58;
    msg_body["mn"] = merger_nonce;

    msg_challenge.type = MessageType::MSG_CHALLENGE;
    msg_challenge.body = msg_body;
    msg_challenge.receivers = {b58_recv_id};

    return msg_challenge;
  }

  template <MessageType T, typename = enable_if_t<T == MessageType::MSG_RESPONSE_2>>
  static auto build(const string &timestamp, const string &b58_recv_id, const string &dhx, const string &dhy, const string &cert, const string &sig) {
    OutNetMsg msg_response2;
    nlohmann::json msg_body;

    msg_body["time"] = timestamp;
    msg_body["dh"]["x"] = dhx;
    msg_body["dh"]["y"] = dhy;
    msg_body["merger"]["id"] = MY_ID_BASE58;
    msg_body["merger"]["cert"] = cert;
    msg_body["merger"]["sig"] = sig;

    msg_response2.type = MessageType::MSG_RESPONSE_2;
    msg_response2.body = msg_body;
    msg_response2.receivers = {b58_recv_id};

    return msg_response2;
  }

  template <MessageType T, typename = enable_if_t<T == MessageType::MSG_ACCEPT>>
  static auto build(const string &b58_recv_id) {
    OutNetMsg msg_accept;
    nlohmann::json msg_body;

    msg_body["time"] = TimeUtil::now();
    msg_body["user"] = b58_recv_id;
    msg_body["val"] = true;

    msg_accept.type = MessageType::MSG_ACCEPT;
    msg_accept.body = msg_body;
    msg_accept.receivers = {b58_recv_id};

    return msg_accept;
  }
};

} // namespace net_plugin
} // namespace gruut