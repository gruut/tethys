#include "include/net_plugin.hpp"
#include "include/rpc_server.hpp"
#include "config/include/network_config.hpp"

#include "../../../include/json.hpp"
#include <unordered_map>
#include <boost/asio/steady_timer.hpp>

namespace gruut {
  using namespace std;
  using namespace nlohmann;
  using namespace net_plugin;

  const auto CONNECTION_CHECK_PERIOD = std::chrono::seconds(30);

  class NetPluginImpl {
  public:
    GruutGeneralService::AsyncService general_service;
    KademliaService::AsyncService kademlia_service;

    string p2p_address;

    unique_ptr<Server> server;
    unique_ptr<ServerCompletionQueue> completion_queue;

    shared_ptr<SignerConnTable> signer_conn_table;
    shared_ptr<RoutingTable> routing_table;
    shared_ptr<BroadcastMsgTable> broadcast_check_table;

    unique_ptr<boost::asio::steady_timer> connection_check_timer;

    ~NetPluginImpl() {
      server->shut_down();
      completion_queue->shut_down();
    }

    void initialize() {
      initialize_receiver();
      register_services_in_receiver();

      connection_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
    }

    void initialize_receiver() {
      ServerBuilder builder;

      builder.AddListeningPort(p2p_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&general_service);
      builder.RegisterService(&kademlia_service);

      completion_queue = builder.AddCompletionQueue();
      server = builder.BuildAndStart();
      logger::INFO("Rpc Server listening on {}", p2p_address);
    }

    void register_services_in_receiver() {
      auto address = p2p_address.substr(0, p2p_address.find_first_of(':') + 1);
      auto port = p2p_address.substr(p2p_address.find_last_of(':') + 1);

      Node my_node(Hash<160>::sha1(MY_ID), MY_ID, address, port);
      routing_table = make_shared<RoutingTable>(my_node, KBUCKET_SIZE);

      signer_conn_table = make_shared<SignerConnTable>();
      broadcast_check_table = make_shared<BroadcastMsgTable>();

      new OpenChannel(&general_service, completion_queue.get(), signer_conn_table);
      new GeneralService(&general_service, completion_queue.get(), routing_table, broadcast_check_table);

      new FindNode(&kademlia_service, completion_queue.get(), routing_table);
      new PingPong(&kademlia_service, completion_queue.get(), routing_table);
    }

    void start() {
      start_connection_monitors();
    }

    void start_connection_monitors() {
      connection_check_timer->expires_from_now(CONNECTION_CHECK_PERIOD);
      connection_check_timer->async_wait([this](boost::system::error_code ec) {
        if (!ec) {
          connection_monitor();
        } else {
          logger::ERROR("Error from connection_check_timer: {}", ec.message());
          start_connection_monitors();
        }
      });
    }

    void connection_monitor() {
      // iterating routing table and check status
      rpc_server_ptr->routing_table
    }

    void pingTask(const Node &node) {
      const auto &endpoint = node.getEndpoint();
      rpc_client_ptr->pingReq(endpoint.address, endpoint.port);
      if (!pong.status.ok()) {
        bool evicted = routing_table->peerTimedOut(node);
        if (!evicted)
          pingTask(node);
      }
    }

    void findNeighborsTask(const IdType &id, const HashedIdType &hashed_id) {
      auto target_list = routing_table->findNeighbors(hashed_id, PARALLELISM_ALPHA);

      for (auto &target : target_list) {
        auto endpoint = target.getEndpoint();

        NeighborsData recv_data = rpc_client.findNodeReq(endpoint.address, endpoint.port, id);
        if (!recv_data.status.ok()) {
          bool evicted = routing_table->peerTimedOut(target);
          if (!evicted)
            findNeighborsTask(id, hashed_id);
          else
            return;
        } else {
          for (auto node : recv_data.neighbors) {
            routing_table->addPeer(move(node));
          }
        }
      }
    }

    void refreshBuckets() {
      for (auto bucket = routing_table->begin(); bucket != routing_table->end(); bucket++) {
        auto since_last_update = bucket->timeSinceLastUpdated();

        if (since_last_update > BUCKET_INACTIVE_TIME_BEFORE_REFRESH) {
          if (!bucket->empty()) {
            const auto &node = bucket->selectRandomNode();
            const auto &id = node.getId();
            const auto &hashed_id = node.getIdHash();

            findNeighborsTask(id, hashed_id);
          }
        }
      }
    }

    void scheduleRefreshBuckets() {
      static auto bucket_index = 0U;
      auto bucket = routing_table->cbegin();

      if (bucket_index < routing_table->bucketsCount()) {
        advance(bucket, bucket_index);
        if (!bucket->empty()) {
          const auto &least_recent_node = bucket->leastRecentlySeenNode();
          pingTask(least_recent_node);
        }
        ++bucket_index;
      } else {
        bucket_index = 0;
      }

      refreshBuckets();
    }

    void refreshBroadcastTable() {
      //TODO: gruut util의  Time객체 이용할 것.
      uint64_t now = static_cast<uint64_t>(
              std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count());

      for (auto it = broadcast_check_table->cbegin(); it != broadcast_check_table->cend();) {
        if (abs((int) (now - it->second)) > KEEP_BROADCAST_MSG_TIME) {
          it = broadcast_check_table->erase(it);
        } else {
          ++it;
        }
      }
    }

    template<typename TStub, typename TService>
    unique_ptr<TStub> genStub(const string &addr, const string &port) {
      auto channel = CreateChannel(addr + ":" + port, InsecureChannelCredentials());
      return TService::NewStub(channel);
    }

    PongData pingReq(const string &receiver_addr, const string &receiver_port) {
      auto stub = genStub<KademliaService::Stub, KademliaService>(receiver_addr, receiver_port);
      ClientContext context;

      //TODO : Time stamp 값 현재 시간값 으로 변경해야함.
      Ping ping;
      ping.set_time_stamp(0);
      ping.set_node_id(MY_ID);
      ping.set_version(1);
      ping.set_sender_address(p2p_address);
      ping.set_sender_port(port_num);

      Pong pong;
      grpc::Status status = stub->AsyncPingPongRaw(&context, ping, &pong);

      PongData ret{pong.node_id(),
                   pong.version(),
                   pong.time_stamp(),
                   status};

      return ret;
    }

    NeighborsData findNodeReq(const string &receiver_addr,
                              const string &receiver_port,
                              const IdType &target_id) {

      auto stub = genStub<KademliaService::Stub, KademliaService>(receiver_addr, receiver_port);
      ClientContext context;

      Target target;
      target.set_target_id(target_id);
      target.set_sender_id(MY_ID);
      target.set_sender_address(p2p_address);
      target.set_sender_port(port_num);
      target.set_time_stamp(0);

      Neighbors neighbors;

      grpc::Status status = stub->FindNode(&context, target, &neighbors);
      vector<Node> neighbor_list;

      for (int i = 0; i < neighbors.neighbors_size(); i++) {
        const auto &node = neighbors.neighbors(i);
        Node some_node(Hash<160>::sha1(node.node_id()), node.node_id(), node.address(), node.port());
        neighbor_list.emplace_back(some_node);
      }

      return NeighborsData{neighbor_list, neighbors.time_stamp(), status};
    }

    vector<GeneralData> sendToMerger(vector<IpEndpoint> &addr_list,
                                     string &packed_msg,
                                     const string &msg_id,
                                     bool broadcast) {

      RequestMsg req_msg;
      req_msg.set_message(packed_msg);
      req_msg.set_broadcast(broadcast);

      vector<GeneralData> result_list;
      if (broadcast)
        req_msg.set_message_id(msg_id);

      for (auto &addr : addr_list) {
        auto stub = genStub<GruutGeneralService::Stub, GruutGeneralService>(addr.address, addr.port);
        ClientContext context;

        MsgStatus msg_status;

        grpc::Status status = stub->GeneralService(&context, req_msg, &msg_status);

        GeneralData data;
        data.status = status;
        if (!status.ok()) {
          logger::INFO("Could not send message to {}", addr.address + ":" + addr.port);
        } else {
          data.msg_status = msg_status;
        }
        result_list.emplace_back(data);
      }
      return result_list;
    }

    void sendToSigner(vector<SignerRpcInfo> &signer_list, vector<string> &packed_msg) {
      if (signer_list.size() != packed_msg.size())
        return;

      size_t num_of_signer = signer_list.size();

      for (int i = 0; i < num_of_signer; i++) {
        auto tag = static_cast<Identity *>(signer_list[i].tag_identity);
        ReplyMsg reply;
        reply.set_message(packed_msg[i]);
        if (signer_list[i].send_msg != nullptr)
          signer_list[i].send_msg->Write(reply, tag);
      }
    }
  };

  NetPlugin::NetPlugin() : impl(new NetPluginImpl()) {}

  void NetPlugin::pluginInitialize(const variables_map &options) {
    logger::INFO("NetPlugin Initialize");

    if (options.count("p2p-address") > 0) {
      auto address = options["p2p-address"].as<string>();

      impl->p2p_address = address;
    }

    impl->initialize();
  }

  void NetPlugin::pluginStart() {
    logger::INFO("NetPlugin Start");

    impl->start();
  }

  NetPlugin::~NetPlugin() {
    impl.reset();
  }
}
