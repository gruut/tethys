#pragma once

#include "rpc_services/rpc_services.hpp"
#include "config/network_type.hpp"
#include <vector>

namespace gruut{
namespace net{

class RpcClient{
public:
  RpcClient() = default;

  PongData pingReq(const std::string &receiver_addr, const std::string &receiver_port);
  NeighborsData findNodeReq(const std::string &receiver_addr, const std::string &receiver_port, const IdType &target_id);

  std::vector<GeneralData> sendToMerger(std::vector<IpEndpoint> &addr_list, std::string &packed_msg, const std::string &msg_id = {}, bool broadcast = false);
  void sendToSigner(std::vector<SignerRpcInfo> &signer_list, std::vector<string> &packed_msg);

  void setUp(std::shared_ptr<BroadcastMsgTable> broadcast_check_table);

private:
  std::shared_ptr<BroadcastMsgTable> m_broadcast_check_table;

  template<typename TStub, typename TService>
  std::unique_ptr<TStub> genStub(const std::string &addr, const std::string &port);
};

}
}