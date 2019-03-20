// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: kademlia_service.proto
#ifndef GRPC_kademlia_5fservice_2eproto__INCLUDED
#define GRPC_kademlia_5fservice_2eproto__INCLUDED

#include "kademlia_service.pb.h"

#include <grpcpp/impl/codegen/async_generic_service.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/impl/codegen/rpc_method.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/status.h>
#include <grpcpp/impl/codegen/stub_options.h>
#include <grpcpp/impl/codegen/sync_stream.h>

namespace grpc {
class CompletionQueue;
class Channel;
class ServerCompletionQueue;
class ServerContext;
}  // namespace grpc

namespace kademlia {

class KademliaService final {
 public:
  static constexpr char const* service_full_name() {
    return "kademlia.KademliaService";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    virtual ::grpc::Status FindNode(::grpc::ClientContext* context, const ::kademlia::Target& request, ::kademlia::Neighbors* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Neighbors>> AsyncFindNode(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Neighbors>>(AsyncFindNodeRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Neighbors>> PrepareAsyncFindNode(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Neighbors>>(PrepareAsyncFindNodeRaw(context, request, cq));
    }
    virtual ::grpc::Status PingPong(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::kademlia::Pong* response) = 0;
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Pong>> AsyncPingPong(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Pong>>(AsyncPingPongRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Pong>> PrepareAsyncPingPong(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Pong>>(PrepareAsyncPingPongRaw(context, request, cq));
    }
  private:
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Neighbors>* AsyncFindNodeRaw(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Neighbors>* PrepareAsyncFindNodeRaw(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Pong>* AsyncPingPongRaw(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientAsyncResponseReaderInterface< ::kademlia::Pong>* PrepareAsyncPingPongRaw(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel);
    ::grpc::Status FindNode(::grpc::ClientContext* context, const ::kademlia::Target& request, ::kademlia::Neighbors* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Neighbors>> AsyncFindNode(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Neighbors>>(AsyncFindNodeRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Neighbors>> PrepareAsyncFindNode(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Neighbors>>(PrepareAsyncFindNodeRaw(context, request, cq));
    }
    ::grpc::Status PingPong(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::kademlia::Pong* response) override;
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Pong>> AsyncPingPong(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Pong>>(AsyncPingPongRaw(context, request, cq));
    }
    std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Pong>> PrepareAsyncPingPong(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncResponseReader< ::kademlia::Pong>>(PrepareAsyncPingPongRaw(context, request, cq));
    }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    ::grpc::ClientAsyncResponseReader< ::kademlia::Neighbors>* AsyncFindNodeRaw(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::kademlia::Neighbors>* PrepareAsyncFindNodeRaw(::grpc::ClientContext* context, const ::kademlia::Target& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::kademlia::Pong>* AsyncPingPongRaw(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientAsyncResponseReader< ::kademlia::Pong>* PrepareAsyncPingPongRaw(::grpc::ClientContext* context, const ::kademlia::Ping& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_FindNode_;
    const ::grpc::internal::RpcMethod rpcmethod_PingPong_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status FindNode(::grpc::ServerContext* context, const ::kademlia::Target* request, ::kademlia::Neighbors* response);
    virtual ::grpc::Status PingPong(::grpc::ServerContext* context, const ::kademlia::Ping* request, ::kademlia::Pong* response);
  };
  template <class BaseClass>
  class WithAsyncMethod_FindNode : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_FindNode() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_FindNode() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status FindNode(::grpc::ServerContext* context, const ::kademlia::Target* request, ::kademlia::Neighbors* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestFindNode(::grpc::ServerContext* context, ::kademlia::Target* request, ::grpc::ServerAsyncResponseWriter< ::kademlia::Neighbors>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_PingPong : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithAsyncMethod_PingPong() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_PingPong() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PingPong(::grpc::ServerContext* context, const ::kademlia::Ping* request, ::kademlia::Pong* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPingPong(::grpc::ServerContext* context, ::kademlia::Ping* request, ::grpc::ServerAsyncResponseWriter< ::kademlia::Pong>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_FindNode<WithAsyncMethod_PingPong<Service > > AsyncService;
  template <class BaseClass>
  class WithGenericMethod_FindNode : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_FindNode() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_FindNode() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status FindNode(::grpc::ServerContext* context, const ::kademlia::Target* request, ::kademlia::Neighbors* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_PingPong : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithGenericMethod_PingPong() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_PingPong() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PingPong(::grpc::ServerContext* context, const ::kademlia::Ping* request, ::kademlia::Pong* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_FindNode : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithRawMethod_FindNode() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_FindNode() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status FindNode(::grpc::ServerContext* context, const ::kademlia::Target* request, ::kademlia::Neighbors* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestFindNode(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(0, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_PingPong : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithRawMethod_PingPong() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_PingPong() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status PingPong(::grpc::ServerContext* context, const ::kademlia::Ping* request, ::kademlia::Pong* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPingPong(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncResponseWriter< ::grpc::ByteBuffer>* response, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncUnary(1, context, request, response, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_FindNode : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_FindNode() {
      ::grpc::Service::MarkMethodStreamed(0,
        new ::grpc::internal::StreamedUnaryHandler< ::kademlia::Target, ::kademlia::Neighbors>(std::bind(&WithStreamedUnaryMethod_FindNode<BaseClass>::StreamedFindNode, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_FindNode() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status FindNode(::grpc::ServerContext* context, const ::kademlia::Target* request, ::kademlia::Neighbors* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedFindNode(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::kademlia::Target,::kademlia::Neighbors>* server_unary_streamer) = 0;
  };
  template <class BaseClass>
  class WithStreamedUnaryMethod_PingPong : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service *service) {}
   public:
    WithStreamedUnaryMethod_PingPong() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::internal::StreamedUnaryHandler< ::kademlia::Ping, ::kademlia::Pong>(std::bind(&WithStreamedUnaryMethod_PingPong<BaseClass>::StreamedPingPong, this, std::placeholders::_1, std::placeholders::_2)));
    }
    ~WithStreamedUnaryMethod_PingPong() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status PingPong(::grpc::ServerContext* context, const ::kademlia::Ping* request, ::kademlia::Pong* response) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with streamed unary
    virtual ::grpc::Status StreamedPingPong(::grpc::ServerContext* context, ::grpc::ServerUnaryStreamer< ::kademlia::Ping,::kademlia::Pong>* server_unary_streamer) = 0;
  };
  typedef WithStreamedUnaryMethod_FindNode<WithStreamedUnaryMethod_PingPong<Service > > StreamedUnaryService;
  typedef Service SplitStreamedService;
  typedef WithStreamedUnaryMethod_FindNode<WithStreamedUnaryMethod_PingPong<Service > > StreamedService;
};

}  // namespace kademlia


#endif  // GRPC_kademlia_5fservice_2eproto__INCLUDED