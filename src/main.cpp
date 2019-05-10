#include "nlohmann/json.hpp"
#include <fstream>
#include <unordered_set>

template <typename T>
struct BoxedOptional {

    BoxedOptional() = default;

    BoxedOptional(T value): value{new T{value}} {}

    BoxedOptional(BoxedOptional const & optional) {
        copy_value_from(optional);
    }

    BoxedOptional & operator = (BoxedOptional const & optional) {
        reset();
        copy_value_from(optional);
        return *this;
    }

    BoxedOptional(BoxedOptional && optional) {
        steal_value_from(optional);
    }

    BoxedOptional & operator = (BoxedOptional && optional) {
        reset();
        steal_value_from(optional);
        return *this;
    }

    ~BoxedOptional() {
        reset();
    }

    void reset() {
        if (value) {
            delete value;
            value = nullptr;
        }
    }

    bool has_value() const {
        return value != nullptr;
    }

    explicit operator bool() const {
        return has_value();
    }

    T & operator * () {
        return *value;
    }

    T const & operator * () const {
        return *value;
    }

    T * operator -> () {
        return value;
    }

    T const * operator -> () const {
        return value;
    }

private:
    T * value = nullptr;

    void copy_value_from(BoxedOptional const & optional) {
        if (optional.value) {
            value = new T{*optional.value};
        }
    }

    void steal_value_from(BoxedOptional & optional) {
        if (optional.value) {
            value = optional.value;
            optional.value = nullptr;
        }
    }

};

struct Schema {

    struct Type {

        enum class Kind: uint8_t {
            Scalar, Object, Interface, Union, Enum, InputObject, List, NonNull
        };

        enum class Scalar: uint8_t {
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
            Kind kind;
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

        struct InputValue {
            std::string name;
            std::string description;
            TypeRef type;
            // TODO: Default value
        };

        struct Field {
            std::string name;
            std::string description;
            std::vector<InputValue> args;
            TypeRef type;
            // TODO: Deprecation
        };

        struct EnumValue {
            std::string name;
            std::string description;
            // TODO: Deprecation
        };

        Kind kind;
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

    struct SpecialType {
        std::string name;
    };

    std::optional<SpecialType> queryType;
    std::optional<SpecialType> mutationType;
    std::optional<SpecialType> subscriptionType;
    std::vector<Type> types;
    // TODO: Directives
};

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json & json, std::optional<T> const & opt) {
            if (opt.has_value()) {
                json = *opt;
            } else {
                json = nullptr;
            }
        }

        static void from_json(const json & json, std::optional<T> & opt) {
            if (json.is_null()) {
                opt.reset();
            } else {
                opt = json.get<T>();
            }
        }
    };

    template <typename T>
    struct adl_serializer<BoxedOptional<T>> {
        static void to_json(json & json, BoxedOptional<T> const & opt) {
            if (opt.has_value()) {
                json = *opt;
            } else {
                json = nullptr;
            }
        }

        static void from_json(const json & json, BoxedOptional<T> & opt) {
            if (json.is_null()) {
                opt.reset();
            } else {
                opt = json.get<T>();
            }
        }
    };
}

using Json = nlohmann::json;

NLOHMANN_JSON_SERIALIZE_ENUM(Schema::Type::Kind, {
    {Schema::Type::Kind::Scalar, "SCALAR"},
    {Schema::Type::Kind::Object, "OBJECT"},
    {Schema::Type::Kind::Interface, "INTERFACE"},
    {Schema::Type::Kind::Union, "UNION"},
    {Schema::Type::Kind::Enum, "ENUM"},
    {Schema::Type::Kind::InputObject, "INPUT_OBJECT"},
    {Schema::Type::Kind::List, "LIST"},
    {Schema::Type::Kind::NonNull, "NON_NULL"}
});

template <typename T>
inline void get_value_to(Json const & json, char const * key, T & target) {
    json.at(key).get_to(target);
}

template <typename T>
inline void get_value_to(Json const & json, char const * key, std::optional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}

template <typename T>
inline void get_value_to(Json const & json, char const * key, BoxedOptional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}

template <typename T>
inline void set_value_from(Json & json, char const * key, T const & source) {
    json[key] = source;
}

template <typename T>
inline void set_value_from(Json & json, char const * key, std::optional<T> const & source) {
    if (source) {
        json[key] = *source;
        return;
    }
    auto it = json.find(key);
    if (it != json.end()) {
        json.erase(it);
    }
}

void from_json(Json const & json, Schema::Type::TypeRef & type) {
    get_value_to(json, "kind", type.kind);
    get_value_to(json, "name", type.name);
    get_value_to(json, "ofType", type.ofType);
}

void from_json(Json const & json, Schema::Type::InputValue & input) {
    get_value_to(json, "name", input.name);
    get_value_to(json, "description", input.description);
    get_value_to(json, "type", input.type);
}

void from_json(Json const & json, Schema::Type::Field & field) {
    get_value_to(json, "name", field.name);
    get_value_to(json, "description", field.description);
    get_value_to(json, "args", field.args);
    get_value_to(json, "type", field.type);
}

void from_json(Json const & json, Schema::Type::EnumValue & value) {
    get_value_to(json, "name", value.name);
    get_value_to(json, "description", value.description);
}

void from_json(Json const & json, Schema::Type & type) {
    get_value_to(json, "kind", type.kind);
    get_value_to(json, "name", type.name);
    get_value_to(json, "description", type.description);
    get_value_to(json, "fields", type.fields);
    get_value_to(json, "inputFields", type.inputFields);
    get_value_to(json, "interfaces", type.interfaces);
    get_value_to(json, "enumValues", type.enumValues);
    get_value_to(json, "possibleTypes", type.possibleTypes);
}

void from_json(Json const & json, Schema::SpecialType & special) {
    get_value_to(json, "name", special.name);
}

void from_json(Json const & json, Schema & schema) {
    get_value_to(json, "queryType", schema.queryType);
    get_value_to(json, "mutationType", schema.mutationType);
    get_value_to(json, "subscriptionType", schema.subscriptionType);
    get_value_to(json, "types", schema.types);
}

std::vector<Schema::Type> sortCustomTypesByDependencyOrder(std::vector<Schema::Type> const &types) {
    using namespace std;

    struct TypeWithDependencies {
        Schema::Type type;
        unordered_set<string> dependencies;
    };

    unordered_map<string, unordered_set<string>> typesToDependents;
    unordered_map<string, TypeWithDependencies> typesToDependencies;

    auto isCustomType = [](Schema::Type::Kind kind) {
        switch (kind) {
            case Schema::Type::Kind::Object:
            case Schema::Type::Kind::Interface:
            case Schema::Type::Kind::Union:
            case Schema::Type::Kind::Enum:
            case Schema::Type::Kind::InputObject:
                return true;

            case Schema::Type::Kind::Scalar:
            case Schema::Type::Kind::List:
            case Schema::Type::Kind::NonNull:
                return false;
        }
    };

    for (auto const & type : types) {
        if (!isCustomType(type.kind)) {
            continue;
        }

        unordered_set<string> dependencies;

        auto addDependency = [&](auto const &dependency) {
            if (dependency.name && isCustomType(dependency.kind)) {
                typesToDependents[*dependency.name].insert(type.name);
                dependencies.insert(*dependency.name);
            }
        };

        for (auto const & field : type.fields) {
            addDependency(field.type.underlyingType());

            for (auto const & arg : field.args) {
                addDependency(arg.type.underlyingType());
            }
        }

        for (auto const & field : type.inputFields) {
            addDependency(field.type.underlyingType());
        }

        for (auto const & interface : type.interfaces) {
            addDependency(interface);
        }

        typesToDependencies[type.name] = {type, move(dependencies)};
    }

    vector<Schema::Type> sortedTypes;

    while (!typesToDependencies.empty()) {
        auto const initialCount = typesToDependencies.size();

        vector<string> addedTypeNames;

        for (auto const & pair : typesToDependencies) {
            if (pair.second.dependencies.empty()) {
                sortedTypes.push_back(pair.second.type);
                addedTypeNames.push_back(pair.first);

                for (auto const & dependentName : typesToDependents[pair.first]) {
                    typesToDependencies[dependentName].dependencies.erase(pair.first);
                }
            }
        }

        for (auto const &addedName : addedTypeNames) {
            typesToDependencies.erase(addedName);
        }

        if (typesToDependencies.size() == initialCount) {
            throw runtime_error{"Circular dependencies in schema"};
        }
    }

    return sortedTypes;
}

constexpr size_t spacesPerIndent = 4;
const std::string unknownEnumCase = "Unknown";

std::string indent(size_t indentation) {
    return std::string(indentation * spacesPerIndent, ' ');
}

std::string generateDescription(std::string const & description, size_t indentation) {
    if (description.empty()) {
        return {};
    }

    std::string generated;

    // Insert a line break before comments
    generated += "\n";

    if (description.find('\n') == std::string::npos) {
        generated += indent(indentation) + "// " + description + "\n";
    } else {
        // Use block comments for multiline strings
        generated += indent(indentation) + "/*\n" + indent(indentation);

        for (auto const character : description) {
            if (character == '\n') {
                generated += "\n" + indent(indentation);
                continue;
            }

            generated += character;
        }

        generated += "\n" + indent(indentation) + "*/\n";
    }

    return generated;
}

std::string screamingSnakeCaseToPascalCase(std::string const & snake) {
    std::string pascal;

    bool isFirstInWord = true;
    for (auto const & character : snake) {
        if (character == '_') {
            isFirstInWord = true;
            continue;
        }

        if (isFirstInWord) {
            pascal += toupper(character);
            isFirstInWord = false;
        } else {
            pascal += tolower(character);
        }
    }

    return pascal;
}

std::string generateEnum(Schema::Type const & type, size_t indentation) {
    std::string generated;
    generated += indent(indentation) + "enum class " + type.name + " {\n";

    auto const valueIndentation = indentation + 1;

    for (auto const & value : type.enumValues) {
        generated += generateDescription(value.description, valueIndentation);
        generated += indent(valueIndentation) + screamingSnakeCaseToPascalCase(value.name) + ",\n";
    }

    generated += indent(valueIndentation) + unknownEnumCase + " = -1\n";

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateEnumSerialization(Schema::Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "NLOHMANN_JSON_SERIALIZE_ENUM(" + type.name + ", {\n";

    auto const valueIndentation = indentation + 1;

    generated += indent(valueIndentation) + "{" + type.name + "::" + unknownEnumCase + ", nullptr},\n";

    for (auto const & value : type.enumValues) {
        generated += indent(valueIndentation) + "{" + type.name + "::" + screamingSnakeCaseToPascalCase(value.name) + ", \"" + value.name + "\"},\n";
    }

    generated += indent(indentation) + "});\n\n";

    return generated;
}

Schema::Type::Scalar scalarType(std::string const & name) {
    if (name == "Int") {
        return Schema::Type::Scalar::Int;
    } else if (name == "Float") {
        return Schema::Type::Scalar::Float;
    } else if (name == "String") {
        return Schema::Type::Scalar::String;
    } else if (name == "Boolean") {
        return Schema::Type::Scalar::Boolean;
    } else if (name == "ID") {
        return Schema::Type::Scalar::ID;
    }

    throw std::runtime_error{"Unknown scalar type: " + name};
}

std::string scalarCppType(Schema::Type::Scalar scalar) {
    switch (scalar) {
        case Schema::Type::Scalar::Int:
            return "int32_t";
        case Schema::Type::Scalar::Float:
            return "double";
        case Schema::Type::Scalar::String:
        // TODO: Separate ID type?
        case Schema::Type::Scalar::ID:
            return "std::string";
        case Schema::Type::Scalar::Boolean:
            return "bool";
    }
}

std::string valueCppType(Schema::Type::TypeRef type, bool shouldCheckNullability = true) {
    if (shouldCheckNullability && type.kind != Schema::Type::Kind::NonNull) {
        return "std::optional<" + valueCppType(type, false) + ">";
    }

    switch (type.kind) {
        case Schema::Type::Kind::Object:
        case Schema::Type::Kind::Interface:
        case Schema::Type::Kind::Union:
        case Schema::Type::Kind::Enum:
        case Schema::Type::Kind::InputObject:
            return type.name.value();

        case Schema::Type::Kind::Scalar:
            return scalarCppType(scalarType(type.name.value()));

        case Schema::Type::Kind::List:
            return "std::vector<" + valueCppType(*type.ofType, false) + ">";

        case Schema::Type::Kind::NonNull:
            return valueCppType(*type.ofType, false);
    }
}

template <typename T>
std::string generateField(T const & field, size_t indentation) {
    std::string generated;
    generated += generateDescription(field.description, indentation);
    generated += indent(indentation) + valueCppType(field.type) + " " + field.name + ";\n";
    return generated;
}

std::string generateInterface(Schema::Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "struct " + type.name + " {\n";

    auto const fieldIndentation = indentation + 1;

    for (auto const & field : type.fields) {
        generated += generateField(field, fieldIndentation);
    }

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateObject(Schema::Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "struct " + type.name + " {\n";

    auto const fieldIndentation = indentation + 1;

    for (auto const & field : type.fields) {
        generated += generateField(field, fieldIndentation);
    }

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateInputObject(Schema::Type const & type, size_t indentation) {
    std::string generated;

    generated += indent(indentation) + "struct " + type.name + " {\n";

    auto const fieldIndentation = indentation + 1;

    for (auto const & field : type.inputFields) {
        generated += generateField(field, fieldIndentation);
    }

    generated += indent(indentation) + "};\n\n";

    return generated;
}

std::string generateTypes(Schema const & schema, std::string const & generatedNamespace) {
    auto const sortedTypes = sortCustomTypesByDependencyOrder(schema.types);

    std::string source;

    source +=
R"(// This file was automatically generated and should not be edited.

#include <optional>
#include <vector>
#include "nlohmann/json.hpp"

)";

    source += "namespace " + generatedNamespace + " {\n\n";

    size_t typeIndentation = 1;

    for (auto const & type : sortedTypes) {
        auto isSpecialType = [&](std::optional<Schema::SpecialType> const & special) {
            return special && special->name == type.name;
        };

        switch (type.kind) {
            case Schema::Type::Kind::Object:
                if (isSpecialType(schema.queryType)) {

                } else if (isSpecialType(schema.mutationType)) {

                } else if (isSpecialType(schema.subscriptionType)) {

                } else {
                    source += generateObject(type, typeIndentation);
                }
                break;

            case Schema::Type::Kind::Interface:
                source += generateInterface(type, typeIndentation);
                break;

            case Schema::Type::Kind::Union:
                // TODO: Union support
                break;

            case Schema::Type::Kind::Enum:
                source += generateEnum(type, typeIndentation);
                source += generateEnumSerialization(type, typeIndentation);
                break;

            case Schema::Type::Kind::InputObject:
                source += generateInputObject(type, typeIndentation);
                break;

            case Schema::Type::Kind::Scalar:
            case Schema::Type::Kind::List:
            case Schema::Type::Kind::NonNull:
                break;
        }
    }

    source += "} // namespace " + generatedNamespace + "\n";

    return source;
}

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("Please provide an input schema and namespace\n");
        return 0;
    }

    std::ifstream file{argv[1]};
    auto const json = Json::parse(file);
    Schema schema = json.at("data").at("__schema");

    auto const source = generateTypes(schema, argv[2]);
    std::ofstream out{"Generated.hpp"};
    out << source;
    out.close();

    return 0;
}
