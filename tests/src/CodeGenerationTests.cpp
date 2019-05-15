#include "doctest.h"
#include "CodeGeneration.hpp"

using namespace caffql;

TEST_SUITE("Code Generation") {

    TEST_CASE("custom type sorting") {

        SUBCASE("sorts types so that dependencies are before dependents") {
            Type a{TypeKind::Enum, "A"};
            // Has field of type A
            Type b{TypeKind::Object, "B", "", {Field{TypeRef{TypeKind::Enum, "A"}, "a"}}};
            // Has field of type A and possible type B
            Type c{TypeKind::Interface, "C", "", {Field{TypeRef{TypeKind::Enum, "A"}, "a"}}, {}, {}, {}, {TypeRef{TypeKind::Object, "B"}}};
            // Has field of type [C!]!
            TypeRef nonNullListOfNonNullC{TypeKind::NonNull, {}, TypeRef{TypeKind::List, {}, TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::Interface, "C"}}}};
            Type d{TypeKind::Object, "D", "", {Field{nonNullListOfNonNullC}}};
            // Union of possible types A, B, C, D
            Type e{TypeKind::Union, "E", "", {}, {}, {}, {}, {TypeRef{TypeKind::Enum, "A"}, TypeRef{TypeKind::Object, "B"}, TypeRef{TypeKind::Interface, "C"}, TypeRef{TypeKind::Object, "D"}}};
            // Input Object with input value of type A
            Type f{TypeKind::InputObject, "F", "", {}, {InputValue{TypeRef{TypeKind::Enum, "A"}, "a"}}};
            // Has field of type A with argument of type F
            Type g{TypeKind::Object, "G", "", {Field{TypeRef{TypeKind::Enum, "A"}, "a", "", {InputValue{TypeRef{TypeKind::InputObject, "F"}}}}}};

            auto sorted = sortCustomTypesByDependencyOrder({g, f, e, d, c, b, a});
            CHECK(sorted == std::vector{a, f, g, b, c, d, e});
        }

        SUBCASE("throws on circular type references") {
            Type a{TypeKind::Object, "A", "", {Field{TypeRef{TypeKind::Object, "B"}, "b"}}};
            Type b{TypeKind::Object, "B", "", {Field{TypeRef{TypeKind::Object, "A"}, "a"}}};
            CHECK_THROWS(sortCustomTypesByDependencyOrder({a, b}));
        }

        SUBCASE("filters out non custom types") {
            auto types = sortCustomTypesByDependencyOrder({{TypeKind::Scalar}, {TypeKind::List}, {TypeKind::NonNull}});
            CHECK(types.empty());
        }

    }

}
