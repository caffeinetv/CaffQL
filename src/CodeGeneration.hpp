#pragma once
#include "BoxedOptional.hpp"
#include <unordered_set>

#define CAFFQL_DEFINE_EQUALS(T, equals) \
inline bool operator == (T const & lhs, T const & rhs) { \
equals \
} \
inline bool operator != (T const & lhs, T const & rhs) { \
return !(lhs == rhs); \
} \

namespace caffql {

enum class TypeKind {
    Scalar, Object, Interface, Union, Enum, InputObject, List, NonNull
};

enum class Scalar {
    // 32 bit
    Int,
    // Double
    Float,
    // UTF-8
    String,
    Boolean,
    ID
};

struct TypeRef {
    TypeKind kind;
    std::optional<std::string> name;
    // NonNull and List only
    BoxedOptional<TypeRef> ofType;

    TypeRef const & underlyingType() const {
        if (ofType) {
            return ofType->underlyingType();
        }
        return *this;
    }

};

CAFFQL_DEFINE_EQUALS(TypeRef,
    return lhs.kind == rhs.kind
        && lhs.name == rhs.name
        && lhs.ofType == rhs.ofType;
)

struct InputValue {
    TypeRef type;
    std::string name;
    std::string description;
    // TODO: Default value
};

CAFFQL_DEFINE_EQUALS(InputValue,
    return lhs.type == rhs.type
        && lhs.name == rhs.name
        && lhs.description == rhs.description;
)

struct Field {
    TypeRef type;
    std::string name;
    std::string description;
    std::vector<InputValue> args;
    // TODO: Deprecation
};

CAFFQL_DEFINE_EQUALS(Field,
    return lhs.type == rhs.type
        && lhs.name == rhs.name
        && lhs.description == rhs.description
        && lhs.args == rhs.args;
)

struct EnumValue {
    std::string name;
    std::string description;
    // TODO: Deprecation
};

CAFFQL_DEFINE_EQUALS(EnumValue,
    return lhs.name == rhs.name
        && lhs.description == rhs.description;
)

struct Type {
    TypeKind kind;
    std::string name;
    std::string description;
    // Object and Interface only
    std::vector<Field> fields;
    // InputObject only
    std::vector<InputValue> inputFields;
    // Object only
    std::vector<TypeRef> interfaces;
    // Enum only
    std::vector<EnumValue> enumValues;
    // Interface and Union only
    std::vector<TypeRef> possibleTypes;
};

CAFFQL_DEFINE_EQUALS(Type,
    return lhs.kind == rhs.kind
        && lhs.name == rhs.name
        && lhs.description == rhs.description
        && lhs.fields == rhs.fields
        && lhs.inputFields == rhs.inputFields
        && lhs.interfaces == rhs.interfaces
        && lhs.enumValues == rhs.enumValues
        && lhs.possibleTypes == rhs.possibleTypes;
)

enum class Operation {
    Query, Mutation, Subscription
};

struct Schema {

    struct OperationType {
        std::string name;
    };

    std::optional<OperationType> queryType;
    std::optional<OperationType> mutationType;
    std::optional<OperationType> subscriptionType;
    std::vector<Type> types;
    // TODO: Directives
};

using TypeMap = std::unordered_map<std::string, Type>;

NLOHMANN_JSON_SERIALIZE_ENUM(TypeKind, {
    {TypeKind::Scalar, "SCALAR"},
    {TypeKind::Object, "OBJECT"},
    {TypeKind::Interface, "INTERFACE"},
    {TypeKind::Union, "UNION"},
    {TypeKind::Enum, "ENUM"},
    {TypeKind::InputObject, "INPUT_OBJECT"},
    {TypeKind::List, "LIST"},
    {TypeKind::NonNull, "NON_NULL"}
});

void from_json(Json const & json, TypeRef & type);

void from_json(Json const & json, InputValue & input);

void from_json(Json const & json, Field & field);

void from_json(Json const & json, EnumValue & value);

void from_json(Json const & json, Type & type);

void from_json(Json const & json, Schema::OperationType & operationType);

void from_json(Json const & json, Schema & schema);

std::vector<Type> sortCustomTypesByDependencyOrder(std::vector<Type> const & types);

constexpr size_t spacesPerIndent = 4;
constexpr auto unknownCaseName = "Unknown";
constexpr auto cppJsonTypeName = "Json";
constexpr auto cppIdTypeName = "Id";
constexpr auto grapqlErrorTypeName = "GraphqlError";

std::string indent(size_t indentation);

std::string generateDescription(std::string const & description, size_t indentation);

std::string screamingSnakeCaseToPascalCase(std::string const & snake);

std::string capitalize(std::string string);

std::string uncapitalize(std::string string);

std::string generateEnum(Type const & type, size_t indentation);

std::string generateEnumSerialization(Type const & type, size_t indentation);

Scalar scalarType(std::string const & name);

std::string cppScalarName(Scalar scalar);

std::string cppTypeName(TypeRef const & type, bool shouldCheckNullability = true);

std::string graphqlTypeName(TypeRef const & type);

std::string cppVariant(std::vector<TypeRef> const & possibleTypes, std::string const & unknownTypeName);

std::string generateDeserializationFunctionDeclaration(std::string const & typeName, size_t indentation);

std::string generateFieldDeserialization(Field const & field, size_t indentation);

std::string generateVariantDeserialization(Type const & type, std::string const & constructUnknown, size_t indentation);

std::string generateInterface(Type const & type, size_t indentation);

std::string generateInterfaceUnknownCaseDeserialization(Type const & type, size_t indentation);

std::string generateInterfaceDeserialization(Type const & type, size_t indentation);

std::string generateUnion(Type const & type, size_t indentation);

std::string generateUnionDeserialization(Type const & type, size_t indentation);

std::string generateObject(Type const & type, size_t indentation);

std::string generateObjectDeserialization(Type const & type, size_t indentation);

std::string generateInputObject(Type const & type, size_t indentation);

std::string generateInputObjectSerialization(Type const & type, size_t indentation);

std::string operationQueryName(Operation operation);

struct QueryVariable {
    std::string name;
    TypeRef type;
};

std::string appendNameToVariablePrefix(std::string const & variablePrefix, std::string const & name);

std::string generateQueryFields(Type const & type, TypeMap const & typeMap, std::string const & variablePrefix, std::vector<QueryVariable> & variables, size_t indentation);

std::string generateQueryField(Field const & field, TypeMap const & typeMap, std::string const & variablePrefix, std::vector<QueryVariable> & variables, size_t indentation);

struct QueryDocument {
    std::string query;
    std::vector<QueryVariable> variables;
};

QueryDocument generateQueryDocument(Field const & field, Operation operation, TypeMap const & typeMap, size_t indentation);

std::string generateOperationRequestFunction(Field const & field, Operation operation, TypeMap const & typeMap, size_t indentation);

std::string generateOperationResponseFunction(Field const & field, size_t indentation);

std::string generateOperationType(Field const & field, Operation operation, TypeMap const & typeMap, size_t indentation);

std::string generateOperationTypes(Type const & type, Operation operation, TypeMap const & typeMap, size_t indentation);

std::string generateGraphqlErrorType(size_t indentation);

std::string generateGraphqlErrorDeserialization(size_t indentation);

enum class AlgebraicNamespace {
    Std,
    Absl
};

std::string algrebraicNamespaceName(AlgebraicNamespace algebraicNamespace);

std::string generateTypes(Schema const & schema, std::string const & generatedNamespace, AlgebraicNamespace algebraicNamespace);

}
