#pragma once
#include "Json.hpp"
#include <optional>

namespace caffql {

template <typename T>
struct BoxedOptional {

    BoxedOptional() = default;

    BoxedOptional(T value): value{std::make_unique<T>(std::move(value))} {}

    BoxedOptional(BoxedOptional const & optional) {
        *this = optional;
    }

    BoxedOptional & operator = (BoxedOptional const & optional) {
        reset();
        if (optional) {
            value = std::make_unique<T>(*optional.value);
        }
        return *this;
    }

    BoxedOptional(BoxedOptional && optional) {
        *this = std::move(optional);
    }

    BoxedOptional & operator = (BoxedOptional && optional) {
        reset();
        if (optional) {
            value = std::move(optional.value);
        }
        return *this;
    }

    void reset() {
        value.reset();
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
        return value.get();
    }

    T const * operator -> () const {
        return value.get();
    }

private:
    std::unique_ptr<T> value;
};

template <typename T> bool operator == (BoxedOptional<T> const & lhs, BoxedOptional<T> const & rhs) {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    return *lhs == *rhs;
}

template <typename T> bool operator != (BoxedOptional<T> const & lhs, BoxedOptional<T> const & rhs) {
    return !(lhs == rhs);
}

}

namespace nlohmann {
    template <typename T>
    struct adl_serializer<caffql::BoxedOptional<T>> {
        static void to_json(json & json, caffql::BoxedOptional<T> const & opt) {
            if (opt.has_value()) {
                json = *opt;
            } else {
                json = nullptr;
            }
        }

        static void from_json(const json & json, caffql::BoxedOptional<T> & opt) {
            if (json.is_null()) {
                opt.reset();
            } else {
                opt = json.get<T>();
            }
        }
    };
}

namespace caffql {

template <typename T>
void get_value_to(Json const & json, char const * key, BoxedOptional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}

}
