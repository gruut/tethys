#include "include/net_plugin.hpp"
#include "include/http_client.hpp"
#include "config/include/network_config.hpp"
#include "rpc_services/include/rpc_services.hpp"
#include "../../../include/json.hpp"
#include <unordered_map>
#include <future>
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
    string tracker_address;

    unique_ptr<Server> server;
    unique_ptr<ServerCompletionQueue> completion_queue;

    shared_ptr<SignerConnTable> signer_conn_table;
    shared_ptr<RoutingTable> routing_table;
    shared_ptr<BroadcastMsgTable> broadcast_check_table;

    unique_ptr<boost::asio::steady_timer> connection_check_timer;

    ~NetPluginImpl() {
      server->Shutdown();
      completion_queue->Shutdown();
    }

    void initialize() {
      initializeReceiver();
      registerServicesInReceiver();

      connection_check_timer = make_unique<boost::asio::steady_timer>(app().getIoContext());
    }

    void initializeReceiver() {
      ServerBuilder builder;

      builder.AddListeningPort(p2p_address, grpc::InsecureServerCredentials());
      builder.RegisterService(&general_service);
      builder.RegisterService(&kademlia_service);

      completion_queue = builder.AddCompletionQueue();
      server = builder.BuildAndStart();
      logger::INFO("Rpc Server listening on {}", p2p_address);
    }

    void registerServicesInReceiver() {
      auto [host, port] = getHostAndPort(p2p_address);

      Node my_node(Hash<160>::sha1(MY_ID), MY_ID, host, port);
      routing_table = make_shared<RoutingTable>(my_node, KBUCKET_SIZE);

      signer_conn_table = make_shared<SignerConnTable>();
      broadcast_check_table = make_shared<BroadcastMsgTable>();

      new OpenChannel(&general_service, completion_queue.get(), signer_conn_table);
      new GeneralService(&general_service, completion_queue.get(), routing_table, broadcast_check_table);

      new FindNode(&kademlia_service, completion_queue.get(), routing_table);
      new PingPong(&kademlia_service, completion_queue.get(), routing_table);
    }

    void start() {
      getPeersFromTracker();
      startConnectionMonitors();
    }

    void getPeersFromTracker() {
      if(!tracker_address.empty()) {
        logger::INFO("Start to get peers list from tracker");

        auto [addr, port] = getHostAndPort(tracker_address);
        auto [_, my_port] = getHostAndPort(p2p_address);

        HttpClient http_client(addr, port);

        auto res = http_client.get("/announce", "port=" + my_port);
        logger::INFO("Get a response from a tracker : {}", res);
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
          auto nodes = bucket.selectAliveNodes();

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
      target.set_sender_id(MY_ID);
      target.set_sender_address(receiver_addr);
      target.set_sender_port(receiver_port);
      target.set_time_stamp(0);

      ClientContext context;
      Neighbors neighbors;
      grpc::Status status = stub->FindNode(&context, target, &neighbors);

      vector<Node> neighbor_list;
      for (int i = 0; i < neighbors.neighbors_size(); i++) {
        const auto &node = neighbors.neighbors(i);
        neighbor_list.emplace_back(Node(Hash<160>::sha1(node.node_id()), node.node_id(), node.address(), node.port()));
      }

      return NeighborsData{neighbor_list, neighbors.time_stamp(), status};
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
