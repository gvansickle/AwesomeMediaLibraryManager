#include <sqlite_orm/sqlite_orm.h>
#include <catch2/catch_all.hpp>

#include <type_traits>  //  std::is_same
#include <string>  //  std::string
#include <tuple>  //  std::tuple

using namespace sqlite_orm;

TEST_CASE("same_or_void") {
    using internal::common_type_of_t;
    using internal::same_or_void;

    //  one argument
    STATIC_REQUIRE(std::is_same<same_or_void<int>::type, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<std::string>::type, std::string>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<long>::type, long>::value);

    //  two arguments
    STATIC_REQUIRE(std::is_same<same_or_void<int, int>::type, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<int, long>::type, void>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<std::string, std::string>::type, std::string>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<std::string, short>::type, void>::value);

    //  three arguments
    STATIC_REQUIRE(std::is_same<same_or_void<int, int, int>::type, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<long, long, long>::type, long>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<int, int, long>::type, void>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<long, int, int>::type, void>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<long, int, long>::type, void>::value);

    //  four arguments
    STATIC_REQUIRE(std::is_same<same_or_void<int, int, int, int>::type, int>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<long, long, long, long>::type, long>::value);
    STATIC_REQUIRE(std::is_same<same_or_void<int, int, int, long>::type, void>::value);

    //  type pack, e.g. tuple
    STATIC_REQUIRE(std::is_same<common_type_of_t<std::tuple<int, int>>, int>::value);
    STATIC_REQUIRE(std::is_same<common_type_of_t<std::tuple<int, long>>, long>::value);
    STATIC_REQUIRE(
        std::is_same<polyfill::detected_t<common_type_of_t, std::tuple<int, const char*>>, polyfill::nonesuch>::value);
}
