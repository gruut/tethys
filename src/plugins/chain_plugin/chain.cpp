#include "include/chain.hpp"

namespace gruut {
Chain::Chain(string dbms, string table_name, string db_user_id, string db_password)
    : db_session(dbms, "service=" + table_name + " user=" + db_user_id + " password=" + db_password) {}
} // namespace gruut
