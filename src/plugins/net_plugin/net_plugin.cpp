#include "include/net_plugin.hpp"
#include "include/http_client.hpp"
#include "config/include/network_config.hpp"
#include "rpc_services/include/rpc_services.hpp"
#include "../../../lib/json/include/json.hpp"
#include <unordered_map>
#include <future>
#include <boost/asio/steady_timer.hpp>
#include <exception>
#include <stdexcept>

namespace gruut {
  using namespace std;
  using namespace net_plugin;

  const auto CONNECTION_CHECK_PERIOD = std::chrono::seconds(30);
  const auto NET_MESSAGE_CHECK_PERIOD = std::chrono::milliseconds(1);

  class NetPluginImpl {
  public:
    GruutGeneralService::AsyncService general_service;
    KademliaService::AsyncService kademlia_service;

    string p2p_address;

    string tracker_address;

    unique_ptr<Server> server;
    unique_ptr<ServerCompletionQueue> completion_queue;

    shared_ptr<SignerConnTable> signer_conn_table;
    shared_ptr<RoutingTable> routing_table;
    shared_ptr<BroadcastMsgTable> broadcast_check_table;

    unique_ptr<boost::asio::steady_timer> connection_check_timer;
    unique_ptr<boost::asio::steady_timer> net_message_check_timer;

    ~NetPluginImpl() {
      server->Shutdown();
      completion_queue->Shutdown();
    }

    void initialize() {
      initializeServer();
      initializeRoutingTable();
      registerServices();

      connection_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
      net_message_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
    }

    void initializeRoutingTable() {
      auto [host, port] = getHostAndPort(p2p_address);

      auto id = p2p_address;
      Node my_node(Hash<160>::sha1(id), id, host, port);
      routing_table = make_shared<RoutingTable>(my_node, KBUCKET_SIZE);
    }

    void initializeServer() {
      ServerBuilder builder;

      builder.AddListeningPort(p2p_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&general_service);
      builder.RegisterService(&kademlia_service);

      completion_queue = builder.AddCompletionQueue();
      server = builder.BuildAndStart();

      logger::INFO("Rpc Server listening on {}", p2p_address);
    }

    void registerServices() {
      signer_conn_table = make_shared<SignerConnTable>();
      broadcast_check_table = make_shared<BroadcastMsgTable>();

      new OpenChannel(&general_service, completion_queue.get(), signer_conn_table);
      new GeneralService(&general_service, completion_queue.get(), routing_table, broadcast_check_table);

      new FindNode(&kademlia_service, completion_queue.get(), routing_table);
      new PingPong(&kademlia_service, completion_queue.get(), routing_table);
    }

    void start() {
      monitorCompletionQueue();
      getPeersFromTracker();
      startConnectionMonitors();
    }

    void monitorCompletionQueue() {
      net_message_check_timer->expires_from_now(NET_MESSAGE_CHECK_PERIOD);
      net_message_check_timer->async_wait([this](boost::system::error_code ec) {
        if(!ec) {
          void *tag;
          bool ok;

          gpr_timespec deadline;
          deadline.clock_type = GPR_TIMESPAN;
          deadline.tv_sec = 0;
          deadline.tv_nsec = 5;

          auto queue_state = completion_queue->AsyncNext(&tag, &ok, deadline);
          if(ok && queue_state == CompletionQueue::NextStatus::GOT_EVENT) {
            static_cast<CallData *>(tag)->proceed();
          }
        } else {
          logger::ERROR("Error from net_message_check_timer: {}", ec.message());
        }

        monitorCompletionQueue();
      });
    }

    void getPeersFromTracker() {
      if(!tracker_address.empty()) {
        logger::INFO("Start to get peers list from tracker");

        auto [addr, port] = getHostAndPort(tracker_address);
        auto [my_host, my_port] = getHostAndPort(p2p_address);

        HttpClient http_client(addr, port);

        auto res = http_client.get("/announce", "port=" + my_port);
        if(!res.empty()) {
          logger::INFO("Get a response from a tracker : {}", res);
          auto peers = json::parse(res);

          if(!json::is_empty(peers)) {
            for(auto& peer : peers) {
              auto [peer_host, peer_port] = getHostAndPort(peer);
              auto id = peer_host + ":" + peer_port;

              if(id != getMyId()) {
                Node node = Node(Hash<160>::sha1(id), id, peer_host, peer_port);
                routing_table->addPeer(move(node));
              }
            }
          }
        }
      }
    }

    void startConnectionMonitors() {
      connection_check_timer->expires_from_now(CONNECTION_CHECK_PERIOD);
      connection_check_timer->async_wait([this](boost::system::error_code ec) {
        if (!ec) {
          refreshBuckets();
        } else {
          logger::ERROR("Error from connection_check_timer: {}", ec.message());
          startConnectionMonitors();
        }
      });
    }

    void refreshBuckets() {
      logger::INFO("Start to refresh buckets");

      for (auto &bucket : *routing_table) {
        if (!bucket.empty()) {
          // TODO(FIX IT): 'removeDeadNodes' flushes all nodes in the table. Because The connection is not established immediately after opening the channel(the channel state would be 'GRPC_CHANNEL_CONNECTING or GRPC_CHANNEL_IDLE.)
          //bucket.removeDeadNodes();
          auto nodes = bucket.selectAliveNodes(true);
          async(launch::async, &NetPluginImpl::findNeighbors, this, nodes);
        }
      }

      startConnectionMonitors();
    }

    void findNeighbors(vector<Node> nodes) {
      for (auto &node : nodes) {
        auto endpoint = node.getEndpoint();

        NeighborsData recv_data = queryNeighborNodes(endpoint.address, endpoint.port, node.getId(), node.getChannelPtr());
        if (!recv_data.status.ok()) {
          bool evicted = routing_table->peerTimedOut(node);
          if(!evicted) {
            queryNeighborNodes(endpoint.address, endpoint.port, node.getId(), node.getChannelPtr());
          }
        } else {
          for (auto &neighbor : recv_data.neighbors) {
            routing_table->addPeer(move(neighbor));
          }
        }
      }
    }

    template<typename TStub, typename TService>
    unique_ptr<TStub> genStub(shared_ptr<grpc::Channel> channel) {
      return TService::NewStub(channel);
    }

    NeighborsData
    queryNeighborNodes(const string &receiver_addr, const string &receiver_port, const IdType &target_id, std::shared_ptr<grpc::Channel> channel) {
      auto stub = genStub<KademliaService::Stub, KademliaService>(channel);

      Target target;
      target.set_target_id(target_id);
      target.set_sender_id(getMyId());

      auto [host, port] = getHostAndPort(p2p_address);
      target.set_sender_address(host);
      target.set_sender_port(port);
      target.set_time_stamp(0);
      
      ClientContext context;
      Neighbors neighbors;
      grpc::Status status = stub->FindNode(&context, target, &neighbors);

      vector<Node> neighbor_list;
      for (int i = 0; i < neighbors.neighbors_size(); i++) {
        const auto &node = neighbors.neighbors(i);

        if(getMyId() != node.node_id()) {
          neighbor_list.emplace_back(Node(Hash<160>::sha1(node.node_id()), node.node_id(), node.address(), node.port()));
        }
      }

      return NeighborsData{neighbor_list, neighbors.time_stamp(), status};
    }

    string getMyId() {
      return p2p_address;
    }

    pair<string, string> getHostAndPort(const string addr) {
      auto host = addr.substr(0, addr.find_first_of(':'));
      auto port = addr.substr(addr.find_last_of(':') + 1);

      return pair<string, string>(host, port);
    }
  };

  NetPlugin::NetPlugin() : impl(new NetPluginImpl()) {}

  void NetPlugin::pluginInitialize(const variables_map &options) {
    logger::INFO("NetPlugin Initialize");

    if (options.count("p2p-address") > 0) {
      auto address = options["p2p-address"].as<string>();

      impl->p2p_address = address;
    }

    if(options.count("tracker-address")) {
      auto tracker_address = options["tracker-address"].as<string>();

      impl->tracker_address = tracker_address;
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
