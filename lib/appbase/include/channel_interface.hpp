#pragma once

#include "channel.hpp"

struct TempData {

};

namespace appbase {
  namespace channels {
    using temp_channel = ChannelTypeTemplate<TempData>;
  }
}