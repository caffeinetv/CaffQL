#include "doctest.h"
#include "CodeGeneration.hpp"

using namespace caffql;

TEST_SUITE_BEGIN("Code Generation");

TEST_CASE("custom type sorting") {

    SUBCASE("sorts types so that dependencies are before dependents and subsorts alphabetaically") {
        Type a{TypeKind::Enum, "A"};
        // Has field of type A
        Type b{TypeKind::Object, "B", "", {Field{a, "a"}}};
        // Has field of type A and possible type B
        Type c{TypeKind::Interface, "C", "", {Field{a, "a"}}, {}, {}, {}, {b}};
        // Has field of type [C!]!
        TypeRef nonNullListOfNonNullC{TypeKind::NonNull, {}, TypeRef{TypeKind::List, {}, TypeRef{TypeKind::NonNull, {}, {c}}}};
        Type d{TypeKind::Object, "D", "", {Field{nonNullListOfNonNullC}}};
        // Union of possible types A, B, C, D
        Type e{TypeKind::Union, "E", "", {}, {}, {}, {}, {a, b, c, d}};
        // Input Object with input value of type A
        Type f{TypeKind::InputObject, "F", "", {}, {InputValue{a, "a"}}};
        // Has field of type A with argument of type F
        Type g{TypeKind::Object, "G", "", {Field{a, "a", "", {InputValue{f}}}}};

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

TEST_CASE("cpp type name") {
    TypeRef objectType{TypeKind::Object, "Object"};
    CHECK(cppTypeName(objectType) == "optional<Object>");
    CHECK(cppTypeName(TypeRef{TypeKind::NonNull, {}, objectType}) == "Object");
    CHECK(cppTypeName(TypeRef{TypeKind::List, {}, objectType}) == "optional<std::vector<optional<Object>>>");
    CHECK(cppTypeName(TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::List, {}, TypeRef{TypeKind::NonNull, {}, objectType}}}) == "std::vector<Object>");
}

TEST_CASE("graphql type name") {
    TypeRef objectType{TypeKind::Object, "Object"};
    CHECK(graphqlTypeName(objectType) == "Object");
    CHECK(graphqlTypeName(TypeRef{TypeKind::NonNull, {}, objectType}) == "Object!");
    CHECK(graphqlTypeName(TypeRef{TypeKind::List, {}, objectType}) == "[Object]");
    CHECK(graphqlTypeName(TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::List, {}, TypeRef{TypeKind::NonNull, {}, objectType}}}) == "[Object!]!");
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

TEST_CASE("query field generation") {
    TypeMap typeMap;
    std::vector<QueryVariable> variables;

    SUBCASE("scalar field") {
        Field field{TypeRef{TypeKind::Scalar, "Int"}, "field"};
        CHECK(generateQueryField(field, typeMap, "", variables, 0) == "field\n");
    }

    SUBCASE("enum field") {
        Field field{TypeRef{TypeKind::Enum, "Enum"}, "field"};
        CHECK(generateQueryField(field, typeMap, "", variables, 0) == "field\n");
    }

    SUBCASE("object field") {
        Type subobjectType{TypeKind::Object, "Subobject"};
        subobjectType.fields = {
            Field{TypeRef{TypeKind::Scalar, "Float"}, "floatField"}
        };

        Type objectType{TypeKind::Object, "Object"};
        objectType.fields = {
            Field{TypeRef{TypeKind::Scalar, "Int"}, "intField"},
            Field{subobjectType, "subobjectField"}
        };

        typeMap = {{"Object", objectType}, {"Subobject", subobjectType}};

        Field field{objectType, "field"};

        auto expected = R"(
        field {
            intField
            subobjectField {
                floatField
            }
        }
)";

        CHECK("\n" + generateQueryField(field, typeMap, "", variables, 2) == expected);
    }

    SUBCASE("interface field") {
        Type interfaceType{TypeKind::Interface, "Interface"};
        interfaceType.fields = {
            Field{TypeRef{TypeKind::Scalar, "Int"}, "intField"}
        };

        Type impA{TypeKind::Object, "ImpA"};
        impA.fields = interfaceType.fields;
        impA.fields.push_back(Field{TypeRef{TypeKind::Scalar, "Float"}, "floatField"});

        Type impB{TypeKind::Object, "ImpB"};
        impB.fields = interfaceType.fields;

        interfaceType.possibleTypes = {impA, impB};

        typeMap = {{"Interface", interfaceType}, {"ImpA", impA}, {"ImpB", impB}};

        Field field{interfaceType, "field"};

        auto expected = R"(
        field {
            __typename
            ...on ImpA {
                intField
                floatField
            }
            ...on ImpB {
                intField
            }
        }
)";

        CHECK("\n" + generateQueryField(field, typeMap, "", variables, 2) == expected);
    }

    SUBCASE("union field") {
        Type unionType{TypeKind::Union, "Union"};

        Type impA{TypeKind::Object, "ImpA"};
        impA.fields = {
            Field{TypeRef{TypeKind::Scalar, "Int"}, "intField"}
        };

        Type impB{TypeKind::Object, "ImpB"};
        impB.fields = {
            Field{TypeRef{TypeKind::Scalar, "Float"}, "floatField"}
        };

        unionType.possibleTypes = {impA, impB};

        typeMap = {{"Union", unionType}, {"ImpA", impA}, {"ImpB", impB}};

        Field field{unionType, "field"};

        auto expected = R"(
        field {
            __typename
            ...on ImpA {
                intField
            }
            ...on ImpB {
                floatField
            }
        }
)";

        CHECK("\n" + generateQueryField(field, typeMap, "", variables, 2) == expected);
    }

    SUBCASE("arguments") {
        Field field{TypeRef{TypeKind::Scalar, "Int"}, "field"};

        field.args = {
            InputValue{TypeRef{TypeKind::Scalar, "Int"}, "argA"},
            InputValue{TypeRef{TypeKind::NonNull, {}, TypeRef{TypeKind::List, {}, TypeRef{TypeKind::Scalar, "Int"}}}, "argB"}
        };

        auto expected = R"(
        field(
            argA: $argA
            argB: $argB
        )
)";

        CHECK("\n" + generateQueryField(field, typeMap, "", variables, 2) == expected);

        std::vector<QueryVariable> expectedVariables{
            {field.args[0].name, field.args[0].type},
            {field.args[1].name, field.args[1].type}
        };

        CHECK(variables == expectedVariables);
    }

    SUBCASE("nested arguments") {
        Type objectType{TypeKind::Object, "Object"};
        Field nestedField{TypeRef{TypeKind::Scalar, "Int"}, "nestedField"};

        nestedField.args = {
            InputValue{TypeRef{TypeKind::Scalar, "Int"}, "nestedArg"}
        };

        objectType.fields = {nestedField};

        Field field{objectType, "field"};

        typeMap = {{"Object", objectType}};

        auto expected = R"(
        field {
            nestedField(
                nestedArg: $objectNestedFieldNestedArg
            )
        }
)";

        CHECK("\n" + generateQueryField(field, typeMap, "", variables, 2) == expected);

        std::vector<QueryVariable> expectedVariables{
            {"objectNestedFieldNestedArg", nestedField.args[0].type}
        };

        CHECK(variables == expectedVariables);
    }

}

TEST_SUITE_END;
