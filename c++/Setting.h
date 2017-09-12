/*
 * Copyright (c) 2017 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file Setting.h
 * @brief A generic setting abstraction supporting input value validation
 *
 * A setting is a data point (of arbitrary type), that may be set to a value
 * and of which the value can be queried. In addition to these obvious
 * transactions, there is a third action, that sets the value of an instance,
 * if the intended value satisfies an arbitrary validator.
 *
 * The class's get method fetches the setting's current value. The = operator
 * sets the settings's value unconditionally. The set method sets the value, if
 * a validator (set at instance construction time) returns true. The set method
 * returns the value returned by said operator.
 */


#include <functional>
#include "c++/Validator.h"

namespace MicroFrameWork {

template <typename T>
class Setting {
public:
    Setting() : validate(TrivialValidator<T>()){};
    Setting(T v) : validate(TrivialValidator<T>()), value(v){};
    Setting(T v, std::function<bool(T)> vd) : validate(vd), value(v){};
    bool
    set(T v) {
        if (!validate(v))
            return false;
        value = v;
        return true;
    };
    T
    get(void) const {
        return value;
    };
    Setting<T> &
    operator=(T v) {
        value = v;
        return *this;
    }

private:
    std::function<bool(T)> validate;
    T value;
};
};
