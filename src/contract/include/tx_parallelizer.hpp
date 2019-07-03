#ifndef TETHYS_SCE_TX_PARALLELIZER_HPP
#define TETHYS_SCE_TX_PARALLELIZER_HPP

#include "../../../lib/tethys-utils/src/type_converter.hpp"
#include "config.hpp"
#include "types.hpp"
#include <vector>

namespace tethys::tsce {

class TxParallelizer {

public:
  TxParallelizer() = default;

  std::vector<std::vector<TransactionJson>> parallelize(BlockJson &block);
};

} // namespace tethys::tsce

#endif
