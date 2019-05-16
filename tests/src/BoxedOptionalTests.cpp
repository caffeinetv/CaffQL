#include "doctest.h"
#include "BoxedOptional.hpp"

using namespace caffql;

TEST_SUITE_BEGIN("BoxedOptional");

TEST_CASE("default construction does not have value") {
    BoxedOptional<int> optional;
    CHECK_FALSE(optional);
}

TEST_CASE("constructing with a value allocates a copy of that value") {
    int value = 1;
    BoxedOptional<int> optional{value};
    CHECK(optional);
    CHECK(*optional == value);
    CHECK(&*optional != &value);
}

TEST_CASE("copy constructing allocates a copy of the other's value") {
    BoxedOptional<int> a{2};
    auto b = a;
    CHECK(*b == *a);
    CHECK(&*b != &*a);
}

TEST_CASE("move constructing transfers the allocated value") {
    BoxedOptional<int> a{0};
    auto address = &*a;
    auto b = std::move(a);
    CHECK_FALSE(a);
    CHECK(&*b == address);
}

TEST_CASE("equality") {
    CHECK(BoxedOptional<int>(5) == BoxedOptional<int>(5));
    CHECK(BoxedOptional<int>() == BoxedOptional<int>());
    CHECK(BoxedOptional<int>(5) != BoxedOptional<int>(6));
    CHECK(BoxedOptional<int>(5) != BoxedOptional<int>());
}

TEST_CASE("deserialization") {

    SUBCASE("from value") {
        BoxedOptional<std::string> x(Json("test"));
        CHECK(*x == "test");
    }

    SUBCASE("from null") {
        BoxedOptional<std::string> x(Json(nullptr));
        CHECK_FALSE(x);
    }

}

TEST_SUITE_END;
