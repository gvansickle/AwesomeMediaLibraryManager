#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer arithmetic operators") {
    internal::db_objects_tuple<> storage;
    internal::serializer_context<internal::db_objects_tuple<>> context{storage};
    std::string value;
    decltype(value) expected;
    SECTION("unary minus") {
        SECTION("func") {
            value = serialize(minus(20), context);
        }
        SECTION("operator") {
            value = serialize(-c(20), context);
        }
        expected = "-20";
    }
    SECTION("add") {
        SECTION("func") {
            value = serialize(add(3, 5), context);
        }
        SECTION("operator") {
            value = serialize(c(3) + 5, context);
        }
        expected = "3 + 5";
    }
    SECTION("sub") {
        SECTION("func") {
            value = serialize(sub(5, -9), context);
        }
        SECTION("operator") {
            value = serialize(c(5) - -9, context);
        }
        expected = "5 - -9";
    }
    SECTION("mul") {
        SECTION("func") {
            value = serialize(mul(10, 0.5), context);
        }
        SECTION("operator") {
            value = serialize(c(10) * 0.5, context);
        }
        expected = "10 * 0.5";
    }
    SECTION("div") {
        SECTION("func") {
            value = serialize(sqlite_orm::div(10, 2), context);
        }
        SECTION("operator") {
            value = serialize(c(10) / 2, context);
        }
        expected = "10 / 2";
    }
    SECTION("mod") {
        SECTION("func") {
            value = serialize(mod(20, 3), context);
        }
        SECTION("operator") {
            value = serialize(c(20) % 3, context);
        }
        expected = "20 % 3";
    }
    SECTION("parentheses keeping order of precedence") {
        SECTION("1") {
            value = serialize(c(4) + 5 + -c(3), context);
            expected = "(4 + 5) + -3";
        }
        SECTION("2") {
            value = serialize(4 + -(c(5) + 3), context);
            expected = "4 + -(5 + 3)";
        }
        SECTION("3") {
            value = serialize(4 + c(5) * 3 + 1, context);
            expected = "(4 + (5 * 3)) + 1";
        }
    }
    REQUIRE(value == expected);
}
