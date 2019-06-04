// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: merger_service.proto

#include "include/merger_service.grpc.pb.h"
#include "include/merger_service.pb.h"

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace grpc_merger {

static const char* TethysMergerService_method_names[] = {
  "/grpc_merger.TethysMergerService/MergerService",
};

std::unique_ptr< TethysMergerService::Stub> TethysMergerService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< TethysMergerService::Stub> stub(new TethysMergerService::Stub(channel));
  return stub;
}

TethysMergerService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_MergerService_(TethysMergerService_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status TethysMergerService::Stub::MergerService(::grpc::ClientContext* context, const ::grpc_merger::RequestMsg& request, ::grpc_merger::MsgStatus* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_MergerService_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::grpc_merger::MsgStatus>* TethysMergerService::Stub::AsyncMergerServiceRaw(::grpc::ClientContext* context, const ::grpc_merger::RequestMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_merger::MsgStatus>::Create(channel_.get(), cq, rpcmethod_MergerService_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::grpc_merger::MsgStatus>* TethysMergerService::Stub::PrepareAsyncMergerServiceRaw(::grpc::ClientContext* context, const ::grpc_merger::RequestMsg& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_merger::MsgStatus>::Create(channel_.get(), cq, rpcmethod_MergerService_, context, request, false);
}

TethysMergerService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      TethysMergerService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< TethysMergerService::Service, ::grpc_merger::RequestMsg, ::grpc_merger::MsgStatus>(
          std::mem_fn(&TethysMergerService::Service::MergerService), this)));
}

TethysMergerService::Service::~Service() {
}

::grpc::Status TethysMergerService::Service::MergerService(::grpc::ServerContext* context, const ::grpc_merger::RequestMsg* request, ::grpc_merger::MsgStatus* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace grpc_merger

