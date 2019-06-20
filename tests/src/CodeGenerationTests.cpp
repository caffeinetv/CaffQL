#include "doctest.h"
#include "CodeGeneration.hpp"

using namespace caffql;

TEST_SUITE_BEGIN("Code Generation");

TEST_CASE("custom type sorting") {

    SUBCASE("sorts types so that dependencies are before dependents and subsorts alphabetaically") {
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
        CHECK(sorted == std::vector{a, b, c, d, e, f, g});
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

TEST_CASE("string conversion functions") {
    CHECK(screamingSnakeCaseToPascalCase("SOME_WORDS_HERE") == "SomeWordsHere");
    CHECK(capitalize("text") == "Text");
    CHECK(uncapitalize("Text") == "text");
}

TEST_CASE("description generation") {

    SUBCASE("empty description generates nothing") {
        CHECK(generateDescription("", 0) == "");
    }

    SUBCASE("Description with no line breaks generates a single-line comment bounded by line breaks") {
        CHECK(generateDescription("Description", 0) == "// Description\n");
    }

    SUBCASE("Description with line breaks generates block comment bounded by line breaks") {
        auto description = "Description\nwith\nlines";
        auto expected = R"(
        /*
        Description
        with
        lines
        */
)";
        CHECK("\n" + generateDescription(description, 2) == expected);
    }

}

TEST_CASE("enum generation") {
    Type enumType{TypeKind::Enum, "EnumType"};
    enumType.enumValues = {{"CASE_ONE"}, {"CASE_TWO", "Description"}};

    SUBCASE("type") {
        std::string expected = R"(
        enum class EnumType {
            CaseOne,
            // Description
            CaseTwo,
            Unknown = -1
        };

)";
        CHECK("\n" + generateEnum(enumType, 2) == expected);
    }

    SUBCASE("serialization") {
        std::string expected = R"(
        NLOHMANN_JSON_SERIALIZE_ENUM(EnumType, {
            {EnumType::Unknown, nullptr},
            {EnumType::CaseOne, "CASE_ONE"},
            {EnumType::CaseTwo, "CASE_TWO"},
        });

)";
        CHECK("\n" + generateEnumSerialization(enumType, 2) == expected);
    }
    
}

TEST_CASE("interface generation") {
    Type interfaceType{TypeKind::Interface, "InterfaceType"};

    interfaceType.fields = {
        Field{TypeRef{TypeKind::NonNull, "", TypeRef{TypeKind::Object, "FieldType"}}, "field"}
    };

    interfaceType.possibleTypes = {
        TypeRef{TypeKind::Object, "A"},
        TypeRef{TypeKind::Object, "B"}
    };

    SUBCASE("type") {
        std::string expected = R"(
        struct UnknownInterfaceType {
            FieldType field;
        };

        struct InterfaceType {
            variant<A, B, UnknownInterfaceType> implementation;

            FieldType const & field() const {
                return visit([](auto const & implementation) -> FieldType const & {
                    return implementation.field;
                }, implementation);
            }

        };

)";
        CHECK("\n" + generateInterface(interfaceType, 2) == expected);
    }

    SUBCASE("deserialization") {
        std::string expected = R"(
        inline void from_json(Json const & json, UnknownInterfaceType & value) {
            json.at("field").get_to(value.field);
        }

        inline void from_json(Json const & json, InterfaceType & value) {
            std::string occupiedType = json.at("__typename");
            if (occupiedType == "A") {
                value = {A(json)};
            } else if (occupiedType == "B") {
                value = {B(json)};
            } else {
                value = {UnknownInterfaceType(json)};
            }
        }

)";
        CHECK("\n" + generateInterfaceDeserialization(interfaceType, 2) == expected);
    }

}

TEST_CASE("union generation") {
    Type unionType{TypeKind::Union, "UnionType"};
    unionType.possibleTypes = {
        TypeRef{TypeKind::Object, "A"},
        TypeRef{TypeKind::Object, "B"}
    };

    SUBCASE("type") {
        std::string expected = R"(
        using UnknownUnionType = monostate;
        using UnionType = variant<A, B, UnknownUnionType>;

)";
        CHECK("\n" + generateUnion(unionType, 2) == expected);
    }

    SUBCASE("deserialization") {
        std::string expected = R"(
        inline void from_json(Json const & json, UnionType & value) {
            std::string occupiedType = json.at("__typename");
            if (occupiedType == "A") {
                value = {A(json)};
            } else if (occupiedType == "B") {
                value = {B(json)};
            } else {
                value = {UnknownUnionType()};
            }
        }

)";
        CHECK("\n" + generateUnionDeserialization(unionType, 2) == expected);
    }

}

TEST_CASE("object generation") {
    Type objectType{TypeKind::Object, "ObjectType"};
    objectType.fields = {
        Field{TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::Object, "FieldType"}}, "field"}
    };

    SUBCASE("type") {
        std::string expected = R"(
        struct ObjectType {
            FieldType field;
        };

)";

        CHECK("\n" + generateObject(objectType, 2) == expected);
    }

    SUBCASE("deserialization") {
        std::string expected = R"(
        inline void from_json(Json const & json, ObjectType & value) {
            json.at("field").get_to(value.field);
        }

)";
        CHECK("\n" + generateObjectDeserialization(objectType, 2) == expected);
    }
}

TEST_CASE("input object generation") {
    Type inputObjectType{TypeKind::InputObject, "InputObjectType"};
    inputObjectType.inputFields = {
        InputValue{TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::InputObject, "InputFieldType"}}, "field"}
    };

    SUBCASE("type") {
        std::string expected = R"(
        struct InputObjectType {
            InputFieldType field;
        };

)";

        CHECK("\n" + generateInputObject(inputObjectType, 2) == expected);
    }

    SUBCASE("serialization") {
        std::string expected = R"(
        inline void to_json(Json & json, InputObjectType const & value) {
            json["field"] = value.field;
        }

)";
        CHECK("\n" + generateInputObjectSerialization(inputObjectType, 2) == expected);
    }
}

TEST_CASE("request function argument passing") {
    SUBCASE("non-string primitive types should be passed by value") {
        CHECK_FALSE(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::Scalar, "Int"}));
        CHECK_FALSE(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::Scalar, "Float"}));
        CHECK_FALSE(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::Scalar, "Boolean"}));
        CHECK_FALSE(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::NonNull, {}, {TypeRef{TypeKind::Scalar, "Int"}}}));
    }

    SUBCASE("string primitive types should be passed by reference") {
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::Scalar, "String"}));
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::Scalar, "ID"}));
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::NonNull, {}, {TypeRef{TypeKind::Scalar, "String"}}}));
    }

    SUBCASE("lists should be passed by reference") {
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::List, {}, {TypeRef{TypeKind::Scalar, "Int"}}}));
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::List, {}, {TypeRef{TypeKind::Scalar, "Int"}}}}));
    }

    SUBCASE("input objects should be passed by reference") {
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::InputObject, "InputType"}));
        CHECK(shouldPassByReferenceToRequestFunction(TypeRef{TypeKind::NonNull, {}, { TypeRef{TypeKind::InputObject, "InputType"}}}));
    }

}

TEST_SUITE_END;
