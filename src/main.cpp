#include "nlohmann/json.hpp"
#include <fstream>

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

    T & operator*() {
        return *value;
    }

    T const & operator*() const {
        return *value;
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
            // Optional
            BoxedOptional<TypeRef> ofType;
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
        std::vector<Field> fields;
        std::vector<InputValue> inputFields;
        std::vector<TypeRef> interfaces;
        std::vector<EnumValue> enumValues;
        std::vector<TypeRef> possibleTypes;
    };

    struct TypeName {
        std::string name;
    };

    std::optional<TypeName> queryType;
    std::optional<TypeName> mutationType;
    std::optional<TypeName> subscriptionType;
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

void from_json(Json const & json, Schema::TypeName & typeName) {
    get_value_to(json, "name", typeName.name);
}

void from_json(Json const & json, Schema & schema) {
    get_value_to(json, "queryType", schema.queryType);
    get_value_to(json, "mutationType", schema.mutationType);
    get_value_to(json, "subscriptionType", schema.subscriptionType);
    get_value_to(json, "types", schema.types);
}

std::string generateTypes(Schema const & schema) {
    std::string source;

    for (auto const & type : schema.types) {
        switch (type.kind) {
            case Schema::Type::Kind::Scalar:

                break;

            case Schema::Type::Kind::Object:

                break;

            case Schema::Type::Kind::Interface:

                break;

            case Schema::Type::Kind::Union:

                break;

            case Schema::Type::Kind::Enum:

                break;

            case Schema::Type::Kind::InputObject:

                break;

            case Schema::Type::Kind::List:

                break;

            case Schema::Type::Kind::NonNull:

                break;
        }
    }

    return source;
}

int main(int argc, const char * argv[]) {
    if (argc < 2) {
        printf("Please provide an input schema\n");
        return 0;
    }

    std::ifstream file{argv[1]};
    auto const json = Json::parse(file);
    Schema schema = json.at("data").at("__schema");

    auto const source = generateTypes(schema);
    std::ofstream out{"Generated.cpp"};
    out << source;
    out.close();

    return 0;
}
