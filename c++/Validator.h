/*
 * Copyright (c) 2017 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file Validator.h
 * @brief Simple data validators
 *
 * These validators take an arbitrary piece of data and return a boolean value.
 */

#ifndef INC_VALIDATOR_H
#define INC_VALIDATOR_H

#include "common/compiler.h"

namespace MicroFrameWork {

template <typename I, typename T>
class Validator {
public:
    Validator() {};
    inline bool operator()(T v) {
        impl().operator()(v);
    }
private:
    I &impl() { return * static_cast<I*>(this); }
};


/**
 * Trivial validator, that always returns true.
 *
 * This is the default validator used by the Setting class.
 */
template <typename T>
class TrivialValidator : public Validator<TrivialValidator<T>, T> {
public:
    bool operator()(UNUSED T v) { return true; }
};

/**
 * Validator for values to fall into a given range
 *
 * Given a value, this operator returns true, if the value falls within a range
 * (defined at instance construction time). The test includes the limits of the
 * range.
 *
 * The data type used with this validator has to have the <= and >= operators
 * defined for it.
 */
template <typename T, T min, T max>
class RangeValidator : public Validator<RangeValidator<T, min, max>, T> {
public:
    RangeValidator() {};
    bool
    operator()(T v) const {
        return v >= min && v <= max;
    };
};

}

#endif /* INC_VALIDATOR_H */
