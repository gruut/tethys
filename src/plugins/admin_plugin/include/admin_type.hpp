#pragma once

namespace gruut {
namespace admin_plugin {

enum class ControlType : int { LOGIN = 1, START = 2 };
enum class ModeType : int { NONE = -1, DEFAULT = 0, MONITOR = 1 };

} // namespace admin_plugin
} // namespace gruut