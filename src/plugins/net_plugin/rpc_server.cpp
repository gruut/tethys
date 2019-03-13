#include "rpc_server.hpp"
#include <thread>

namespace gruut {
namespace net {

void RpcServer::setUp(std::shared_ptr<SignerConnTable> signer_conn_table,
                      std::shared_ptr<RoutingTable> routing_table,
                      std::shared_ptr<BroadcastMsgTable> broadcast_check_table) {

  m_signer_conn_table = std::move(signer_conn_table);
  m_routing_table = std::move(routing_table);
  m_broadcast_check_table = std::move(broadcast_check_table);
}

void RpcServer::initService() {

  new OpenChannel(&m_general_service, m_completion_queue.get(), m_signer_conn_table);
  new GeneralService(&m_general_service, m_completion_queue.get(), m_routing_table, m_broadcast_check_table);

  new FindNode(&m_kademlia_service, m_completion_queue.get(), m_routing_table);
  new PingPong(&m_kademlia_service, m_completion_queue.get(), m_routing_table);
}

void RpcServer::start(){
  void *tag;
  bool ok;

  while (true) {
      GPR_ASSERT(m_completion_queue->Next(&tag, &ok));
      if (ok)
        static_cast<CallData *>(tag)->proceed();
      else
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

}

void RpcServer::run(const std::string &port_num) {
  std::string server_address;
  server_address = "0.0.0.0:" + port_num;

  std::cout<<"Rpc Server listening on"<<server_address<<std::endl;
  m_port_num = port_num;
  ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&m_general_service);

  builder.RegisterService(&m_kademlia_service);

  m_completion_queue = builder.AddCompletionQueue();
  m_server = builder.BuildAndStart();

  initService();
  start();
}
}
}
