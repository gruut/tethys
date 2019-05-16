// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: admin_service.proto

#include "include/admin_service.pb.h"
#include "include/admin_service.grpc.pb.h"

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/method_handler_impl.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace grpc_admin {

static const char* GruutAdminService_method_names[] = {
  "/grpc_admin.GruutAdminService/Setup",
  "/grpc_admin.GruutAdminService/Start",
  "/grpc_admin.GruutAdminService/Stop",
  "/grpc_admin.GruutAdminService/CheckStatus",
};

std::unique_ptr< GruutAdminService::Stub> GruutAdminService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< GruutAdminService::Stub> stub(new GruutAdminService::Stub(channel));
  return stub;
}

GruutAdminService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_Setup_(GruutAdminService_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_Start_(GruutAdminService_method_names[1], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_Stop_(GruutAdminService_method_names[2], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_CheckStatus_(GruutAdminService_method_names[3], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status GruutAdminService::Stub::Setup(::grpc::ClientContext* context, const ::grpc_admin::ReqSetup& request, ::grpc_admin::ResSetup* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_Setup_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResSetup>* GruutAdminService::Stub::AsyncSetupRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqSetup& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResSetup>::Create(channel_.get(), cq, rpcmethod_Setup_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResSetup>* GruutAdminService::Stub::PrepareAsyncSetupRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqSetup& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResSetup>::Create(channel_.get(), cq, rpcmethod_Setup_, context, request, false);
}

::grpc::Status GruutAdminService::Stub::Start(::grpc::ClientContext* context, const ::grpc_admin::ReqStart& request, ::grpc_admin::ResStart* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_Start_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResStart>* GruutAdminService::Stub::AsyncStartRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqStart& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResStart>::Create(channel_.get(), cq, rpcmethod_Start_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResStart>* GruutAdminService::Stub::PrepareAsyncStartRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqStart& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResStart>::Create(channel_.get(), cq, rpcmethod_Start_, context, request, false);
}

::grpc::Status GruutAdminService::Stub::Stop(::grpc::ClientContext* context, const ::grpc_admin::ReqStop& request, ::grpc_admin::ResStop* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_Stop_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResStop>* GruutAdminService::Stub::AsyncStopRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqStop& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResStop>::Create(channel_.get(), cq, rpcmethod_Stop_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResStop>* GruutAdminService::Stub::PrepareAsyncStopRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqStop& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResStop>::Create(channel_.get(), cq, rpcmethod_Stop_, context, request, false);
}

::grpc::Status GruutAdminService::Stub::CheckStatus(::grpc::ClientContext* context, const ::grpc_admin::ReqStatus& request, ::grpc_admin::ResStatus* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_CheckStatus_, context, request, response);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResStatus>* GruutAdminService::Stub::AsyncCheckStatusRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqStatus& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResStatus>::Create(channel_.get(), cq, rpcmethod_CheckStatus_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::grpc_admin::ResStatus>* GruutAdminService::Stub::PrepareAsyncCheckStatusRaw(::grpc::ClientContext* context, const ::grpc_admin::ReqStatus& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderFactory< ::grpc_admin::ResStatus>::Create(channel_.get(), cq, rpcmethod_CheckStatus_, context, request, false);
}

GruutAdminService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      GruutAdminService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< GruutAdminService::Service, ::grpc_admin::ReqSetup, ::grpc_admin::ResSetup>(
          std::mem_fn(&GruutAdminService::Service::Setup), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      GruutAdminService_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< GruutAdminService::Service, ::grpc_admin::ReqStart, ::grpc_admin::ResStart>(
          std::mem_fn(&GruutAdminService::Service::Start), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      GruutAdminService_method_names[2],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< GruutAdminService::Service, ::grpc_admin::ReqStop, ::grpc_admin::ResStop>(
          std::mem_fn(&GruutAdminService::Service::Stop), this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      GruutAdminService_method_names[3],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< GruutAdminService::Service, ::grpc_admin::ReqStatus, ::grpc_admin::ResStatus>(
          std::mem_fn(&GruutAdminService::Service::CheckStatus), this)));
}

GruutAdminService::Service::~Service() {
}

::grpc::Status GruutAdminService::Service::Setup(::grpc::ServerContext* context, const ::grpc_admin::ReqSetup* request, ::grpc_admin::ResSetup* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status GruutAdminService::Service::Start(::grpc::ServerContext* context, const ::grpc_admin::ReqStart* request, ::grpc_admin::ResStart* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status GruutAdminService::Service::Stop(::grpc::ServerContext* context, const ::grpc_admin::ReqStop* request, ::grpc_admin::ResStop* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status GruutAdminService::Service::CheckStatus(::grpc::ServerContext* context, const ::grpc_admin::ReqStatus* request, ::grpc_admin::ResStatus* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace grpc_admin

