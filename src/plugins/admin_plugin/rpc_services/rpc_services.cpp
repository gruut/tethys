#include "include/rpc_services.hpp"
#include "../../../../lib/json/include/json.hpp"

#include <exception>

namespace gruut {
namespace admin_plugin {

using namespace appbase;
using namespace std;

//TODO : handle admin request
template<>
void AdminService<ReqSetup, ResSetup>::proceed() {

}

template<>
void AdminService<ReqStart, ResStart>::proceed() {

}

template<>
void AdminService<ReqStop, ResStop>::proceed(){

}

template<>
void AdminService<ReqStatus, ResStatus>::proceed(){

}


} // namespace admin_plugin
} // namespace gruut