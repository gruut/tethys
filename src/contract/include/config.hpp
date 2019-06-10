#ifndef TETHYS_SCE_CONFIG_HPP
#define TETHYS_SCE_CONFIG_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <optional>
#include <regex>
#include <cctype>
#include <unordered_map>

// common include files
#include "../include/tinyxml2.h"
#include "../include/json.hpp"
#include "../include/date.hpp"

#include "types.hpp"

#include "utils/json.hpp"
#include "misc_tool.hpp"
#include "sha256.hpp"
#include "type_converter.hpp"
#include "bytes_builder.hpp"
#include "ecdsa.hpp"
#include "ags.hpp"

namespace tethys::tsce {

constexpr int NUM_GSCE_THREAD = 4;
constexpr int MAX_INPUT_SIZE = 5;
constexpr int MIN_USER_FEE = 10;

}

#endif //GRUUTSCE_CONFIG_HPP
