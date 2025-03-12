#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

using namespace sqlite_orm;

TEST_CASE("statement_serializer logical operators") {
    struct User {
        int id = 0;
        std::string name;
    };
    auto table = make_table("users", make_column("id", &User::id), make_column("name", &User::name));
    using db_objects_t = internal::db_objects_tuple<decltype(table)>;
    auto dbObjects = db_objects_t{table};
    using context_t = internal::serializer_context<db_objects_t>;
    context_t context{dbObjects};
    std::string stringValue;
    decltype(stringValue) expected;
    SECTION("and") {
        SECTION("simple") {
            SECTION("operator") {
                SECTION("c + 0") {
                    stringValue = internal::serialize(c(0) and 0, context);
                }
                SECTION("0 + c") {
                    stringValue = internal::serialize(0 and c(0), context);
                }
            }
            SECTION("function") {
                stringValue = internal::serialize(and_(0, 0), context);
            }
            SECTION("member function") {
                stringValue = internal::serialize(c(0).and_(0), context);
            }
            expected = "0 AND 0";
        }
        SECTION("complex") {
            SECTION("operators") {
                stringValue = internal::serialize(c(&User::id) == 5 and c(&User::name) == "Ariana", context);
            }
            SECTION("functions") {
                stringValue = internal::serialize(is_equal(&User::id, 5) and is_equal(&User::name, "Ariana"), context);
            }
            expected = R"(("id" = 5) AND ("name" = 'Ariana'))";
        }
    }
    SECTION("or") {
        SECTION("simple") {
            SECTION("function") {
                stringValue = internal::serialize(or_(0, 0), context);
            }
            SECTION("member function") {
                stringValue = internal::serialize(c(0).or_(0), context);
            }
            expected = "0 OR 0";
        }
        SECTION("complex") {
            SECTION("operators") {
                stringValue = internal::serialize(c(&User::id) == 5 or c(&User::name) == "Ariana", context);
            }
            SECTION("functions") {
                stringValue = internal::serialize(is_equal(&User::id, 5) or is_equal(&User::name, "Ariana"), context);
            }
            expected = R"(("id" = 5) OR ("name" = 'Ariana'))";
        }
    }
    SECTION("not") {
        SECTION("simple") {
            SECTION("operator") {
                stringValue = serialize(!c(20), context);
            }
            expected = "NOT 20";
        }
        SECTION("complex") {
            SECTION("operator") {
                stringValue = serialize(!(c(20) and 1), context);
            }
            expected = "NOT (20 AND 1)";
        }
    }
    SECTION("in") {
        SECTION("static in") {
            auto inValue = c(&User::id).in(1, 2, 3);
            stringValue = internal::serialize(inValue, context);
            expected = R"("id" IN (1, 2, 3))";
        }
        SECTION("static not in") {
            auto inValue = c(&User::id).not_in(1, 2, 3);
            stringValue = internal::serialize(inValue, context);
            expected = R"("id" NOT IN (1, 2, 3))";
        }
        SECTION("dynamic in") {
            SECTION("initializer list") {
                auto inValue = in(&User::id, {1, 2, 3});
                stringValue = internal::serialize(inValue, context);
            }
            SECTION("vector") {
                std::vector<int> vector{1, 2, 3};
                auto inValue = in(&User::id, vector);
                stringValue = internal::serialize(inValue, context);
            }
            SECTION("list") {
                std::list<int> list{1, 2, 3};
                auto inValue = in(&User::id, list);
                stringValue = internal::serialize(inValue, context);
            }
            expected = R"("id" IN (1, 2, 3))";
        }
        SECTION("dynamic not in") {
            auto inValue = not_in(&User::id, {1, 2, 3});
            stringValue = internal::serialize(inValue, context);
            expected = R"("id" NOT IN (1, 2, 3))";
        }
    }
    SECTION("parentheses keeping order of precedence") {
        SECTION("1") {
            stringValue = serialize(c(0) and 0, context);
            expected = "0 AND 0";
        }
        SECTION("2") {
            stringValue = serialize(c(0) and 0 or 1, context);
            expected = "(0 AND 0) OR 1";
        }
        SECTION("3") {
            stringValue = serialize(c(0) and 0 or c(1) and 1, context);
            expected = "(0 AND 0) OR (1 AND 1)";
        }
    }
    REQUIRE(stringValue == expected);
}
