#include "include/rpc_services.hpp"
#include "../../../../lib/json/include/json.hpp"

#include <exception>

namespace gruut {
namespace admin_plugin {

using namespace appbase;
using namespace std;

// TODO : handle admin request
template <>
void AdminService<ReqSetup, ResSetup>::proceed() {}

template <>
void AdminService<ReqStart, ResStart>::proceed() {}

template <>
void AdminService<ReqStop, ResStop>::proceed() {}

template <>
void AdminService<ReqStatus, ResStatus>::proceed() {
  switch (receive_status) {
  case AdminRpcCallStatus::CREATE: {
    receive_status = AdminRpcCallStatus::PROCESS;
    service->RequestCheckStatus(&context, &req, &responder, completion_queue, completion_queue, this);
  } break;

  case AdminRpcCallStatus::PROCESS: {
    new AdminService(service, completion_queue);

    res.set_alive(true);

    Status rpc_status = Status::OK;
    receive_status = AdminRpcCallStatus::FINISH;
    responder.Finish(res, Status::OK, this);
  } break;

  default: { delete this; } break;
  }
}

} // namespace admin_plugin
} // namespace gruut
