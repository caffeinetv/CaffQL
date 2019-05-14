#pragma once
#include "Json.hpp"

template <typename T>
struct BoxedOptional {

    BoxedOptional() = default;

    BoxedOptional(T value): value{new T{value}} {}

    BoxedOptional(BoxedOptional const & optional) {
        copyValueFrom(optional);
    }

    BoxedOptional & operator = (BoxedOptional const & optional) {
        reset();
        copyValueFrom(optional);
        return *this;
    }

    BoxedOptional(BoxedOptional && optional) {
        stealValueFrom(optional);
    }

    BoxedOptional & operator = (BoxedOptional && optional) {
        reset();
        stealValueFrom(optional);
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

    void copyValueFrom(BoxedOptional const & optional) {
        if (optional.value) {
            value = new T{*optional.value};
        }
    }

    void stealValueFrom(BoxedOptional & optional) {
        if (optional.value) {
            value = optional.value;
            optional.value = nullptr;
        }
    }

};

namespace nlohmann {
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

template <typename T>
void get_value_to(Json const & json, char const * key, BoxedOptional<T> & target) {
    auto it = json.find(key);
    if (it != json.end()) {
        it->get_to(target);
    } else {
        target.reset();
    }
}
