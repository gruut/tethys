#include "rpc_client.hpp"

namespace gruut{
namespace net{

void RpcClient::setUp(std::shared_ptr<BroadcastMsgTable> broadcast_check_table){

  m_broadcast_check_table  = std::move(broadcast_check_table);
}

template<typename TStub, typename TService>
std::unique_ptr<TStub> RpcClient::genStub(const std::string &addr, const std::string &port) {

  auto credential = InsecureChannelCredentials();
  auto channel = CreateChannel(addr + ":" + port, credential);
  return TService::NewStub(channel);
}

PongData RpcClient::pingReq(const std::string &receiver_addr, const std::string &receiver_port) {

  auto stub = genStub<KademliaService::Stub, KademliaService>(receiver_addr, receiver_port);
  ClientContext context;

  //TODO : Time stamp 값 현재 시간값 으로 변경해야함.
  Ping ping;
  ping.set_time_stamp(0);
  ping.set_node_id(MY_ID);
  ping.set_version(1);
  ping.set_sender_address(IP_ADDRESS);
  ping.set_sender_port(DEFAULT_PORT_NUM);

  Pong pong;
  grpc::Status status = stub->PingPong(&context, ping, &pong);

  PongData ret{pong.node_id(),
               pong.version(),
               pong.time_stamp(),
               status};

  return ret;
}

NeighborsData RpcClient::findNodeReq(const std::string &receiver_addr,
                                  const std::string &receiver_port,
                                  const IdType &target_id) {

  auto stub = genStub<KademliaService::Stub, KademliaService>(receiver_addr, receiver_port);
  ClientContext context;

  Target target;
  target.set_target_id(target_id);
  target.set_sender_id(MY_ID);
  target.set_sender_address(IP_ADDRESS);
  target.set_sender_port(DEFAULT_PORT_NUM);
  target.set_time_stamp(0);

  Neighbors neighbors;

  grpc::Status status = stub->FindNode(&context, target, &neighbors);
  std::vector<Node> neighbor_list;

  for(int i=0; i < neighbors.neighbors_size(); i++) {
    const auto &node = neighbors.neighbors(i);
    Node some_node(Hash<160>::sha1(node.node_id()), node.node_id(), node.address(), node.port());
    neighbor_list.emplace_back(some_node);
  }

  return NeighborsData{ neighbor_list, neighbors.time_stamp(), status};
}

std::vector<GeneralData> RpcClient::sendToMerger(std::vector<IpEndpoint> &addr_list,
                                                  std::string &packed_msg,
                                                  const std::string &msg_id,
                                                  bool broadcast) {

  RequestMsg req_msg;
  req_msg.set_message(packed_msg);
  req_msg.set_broadcast(broadcast);

  std::vector<GeneralData> result_list;
  if(broadcast)
    req_msg.set_message_id(msg_id);

  for(auto &addr : addr_list){
     auto stub = genStub<GruutGeneralService::Stub,GruutGeneralService>(addr.address, addr.port);
     ClientContext context;

     MsgStatus msg_status;

     grpc::Status status = stub->GeneralService(&context, req_msg, &msg_status);

     GeneralData data;
     data.status = status;
     if(!status.ok()){
        std::cout<<"Could not send message to "<<addr.address + ":" + addr.port<<std::endl;
     }
     else{
       data.msg_status = msg_status;
     }
     result_list.emplace_back(data);
  }
  return result_list;
}


void RpcClient::sendToSigner(std::vector<SignerRpcInfo> &signer_list, std::vector<string> &packed_msg){

  if(signer_list.size() != packed_msg.size())
    return;

  size_t num_of_signer = signer_list.size();

  for(int i = 0; i < num_of_signer; i++){
    auto tag = static_cast<Identity *>(signer_list[i].tag_identity);
    ReplyMsg reply;
    reply.set_message(packed_msg[i]);
    if(signer_list[i].send_msg != nullptr)
      signer_list[i].send_msg->Write(reply, tag);
  }
}

}
}