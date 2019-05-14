#pragma once
#include "nlohmann/json.hpp"

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
}

using Json = nlohmann::json;

template <typename T>
void get_value_to(Json const & json, char const * key, T & target) {
    json.at(key).get_to(target);
}

template <typename T>
void get_value_to(Json const & json, char const * key, std::optional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}
