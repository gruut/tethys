#ifndef TETHYS_SCE_TX_PARALLELIZER_HPP
#define TETHYS_SCE_TX_PARALLELIZER_HPP

#include <vector>
#include "config.hpp"
#include "types.hpp"
#include "../../../lib/tethys-utils/src/type_converter.hpp"

namespace tethys::tsce {

  class TxParallelizer {

  public:
    TxParallelizer() = default;

    std::vector<std::vector<TransactionJson>> parallelize(BlockJson &block);
  };

}

#endif
