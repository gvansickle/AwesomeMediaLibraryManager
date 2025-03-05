#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include "prepared_common.h"

#if SQLITE_VERSION_NUMBER >= 3006019
using namespace sqlite_orm;

#ifdef SQLITE_ORM_OPTIONAL_SUPPORTED
TEST_CASE("Prepared get all optional") {
    using namespace PreparedStatementTests;
    using Catch::Matchers::UnorderedEquals;

    const int defaultVisitTime = 50;

    auto filename = "prepared.sqlite";
    remove(filename);
    auto storage = make_storage(filename,
                                make_index("user_id_index", &User::id),
                                make_table("users",
                                           make_column("id", &User::id, primary_key().autoincrement()),
                                           make_column("name", &User::name)),
                                make_table("visits",
                                           make_column("id", &Visit::id, primary_key().autoincrement()),
                                           make_column("user_id", &Visit::userId),
                                           make_column("time", &Visit::time, default_value(defaultVisitTime)),
                                           foreign_key(&Visit::userId).references(&User::id)),
                                make_table("users_and_visits",
                                           make_column("user_id", &UserAndVisit::userId),
                                           make_column("visit_id", &UserAndVisit::visitId),
                                           make_column("description", &UserAndVisit::description),
                                           primary_key(&UserAndVisit::userId, &UserAndVisit::visitId)));
    storage.sync_schema();

    storage.replace(User{1, "Team BS"});
    storage.replace(User{2, "Shy'm"});
    storage.replace(User{3, "Maître Gims"});

    storage.replace(UserAndVisit{2, 1, "Glad you came"});
    storage.replace(UserAndVisit{3, 1, "Shine on"});

    {
        auto statement = storage.prepare(get_all_optional<User>());
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            auto rows = storage.execute(statement);
            const std::vector<User> expected = {{1, "Team BS"}, {2, "Shy'm"}, {3, "Maître Gims"}};
            REQUIRE(rows.size() == expected.size());
            REQUIRE(*rows[0] == expected[0]);
            REQUIRE(*rows[1] == expected[1]);
            REQUIRE(*rows[2] == expected[2]);
        }
    }
    {  //  by val
        auto statement = storage.prepare(get_all_optional<User>(where(less_than(&User::id, 3))));
        using Statement = decltype(statement);
        using Expression = Statement::expression_type;
        using NodeTuple = internal::node_tuple<Expression>::type;
        using BindTuple = internal::bindable_filter_t<NodeTuple>;
        {
            STATIC_REQUIRE(std::tuple_size<BindTuple>::value == 1);
            {
                using Arg0 = std::tuple_element_t<0, BindTuple>;
                STATIC_REQUIRE(std::is_same<Arg0, int>::value);
            }
        }
        REQUIRE(get<0>(statement) == 3);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            {
                auto rows = storage.execute(statement);
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                expected.push_back(User{2, "Shy'm"});
                REQUIRE(rows.size() == expected.size());
                REQUIRE(*rows[0] == expected[0]);
                REQUIRE(*rows[1] == expected[1]);
            }
            {
                get<0>(statement) = 2;
                REQUIRE(get<0>(statement) == 2);
                auto rows = storage.execute(statement);
                std::vector<User> expected;
                expected.push_back(User{1, "Team BS"});
                REQUIRE(rows.size() == expected.size());
                REQUIRE(*rows[0] == expected[0]);
            }
        }
    }
    {  //  by ref
        auto id = 3;
        auto statement = storage.prepare(get_all_optional<User>(where(less_than(&User::id, std::ref(id)))));
        using Statement = decltype(statement);
        using Expression = Statement::expression_type;
        using NodeTuple = internal::node_tuple<Expression>::type;
        using BindTuple = internal::bindable_filter_t<NodeTuple>;
        {
            STATIC_REQUIRE(std::tuple_size<BindTuple>::value == 1);
            {
                using Arg0 = std::tuple_element_t<0, BindTuple>;
                STATIC_REQUIRE(std::is_same<Arg0, int>::value);
            }
        }
        REQUIRE(get<0>(statement) == 3);
        REQUIRE(&get<0>(statement) == &id);
        testSerializing(statement);
        SECTION("nothing") {
            //..
        }
        SECTION("execute") {
            {
                auto rows = storage.execute(statement);
                const std::vector<User> expected = {{1, "Team BS"}, {2, "Shy'm"}};
                REQUIRE(rows.size() == expected.size());
                REQUIRE(*rows[0] == expected[0]);
                REQUIRE(*rows[1] == expected[1]);
            }
            {
                id = 2;
                REQUIRE(get<0>(statement) == 2);
                REQUIRE(&get<0>(statement) == &id);
                auto rows = storage.execute(statement);
                const std::vector<User> expected = {{1, "Team BS"}};
                REQUIRE(rows.size() == expected.size());
                REQUIRE(*rows[0] == expected[0]);
            }
        }
    }
}
#endif  // SQLITE_ORM_OPTIONAL_SUPPORTED
#endif
