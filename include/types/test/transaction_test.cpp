#define CATCH_CONFIG_MAIN

#include <rttr/registration>
#include <iostream>

#include "../../catch.hpp"
#include "../transaction.hpp"

using namespace std;

TEST_CASE("Transaction") {
    SECTION("print all properties") {
        auto t = rttr::type::get<Transaction>();

        for(auto& prop : t.get_properties()) {
            cout << prop.get_name() << endl;
        }
    }
}
