/*
 * Copyright (c) 2017 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file StagedVariantContainer.h
 * @brief A generic three-way container class
 *
 * This data type is based on the StagedContainer class. In addition, it
 * supports a second version of the active data set. This may be useful if the
 * data from the active data set cannot be used as-is in the application. An
 * example may be the setup for a DDS chip, where you may want to configure a
 * start and a stop frequency as well as a time for a ramp to take between the
 * two frequencies. The DDS will likely not be able to set an exact triplet of
 * configuration values from a triplet of floating point values: The chip will
 * likely only be able to setup discrete values.
 *
 * The actualArea of this data structure may be used to reflect such
 * differences between intended setup and possible settings.
 *
 * @code
 * void
 * transform(const int &src, int &dst)
 * {
 *     dst = src - (src % 2);
 * }
 *
 * // ...
 *
 * MicroFrameWork::StagedVariantContainer<int> foo(&transform);
 * foo.staged() = 23;
 * foo.commit();
 * int bar = foo.active();   // bar is now 23.
 * bar = foo.actual();       // bar is now 22.
 * foo.staged() = 42;
 * bar = foo.active();       // bar is still 23; .actual() would still be 22.
 * foo.commit();
 * bar = foo.actual();       // bar is now 42.
 * @endcode
 */

#ifndef INC_STAGEDVARIANTCONTAINER_H
#define INC_STAGEDVARIANTCONTAINER_H

#include <functional>
#include "c++/StagedContainer.h"

namespace MicroFrameWork {

/**
 * A transformer definition for StagedVariantContainer that transforms nothing
 */
template <typename T>
class TrivialTransformer {
public:
    TrivialTransformer(){};
    /**
     * TrivialTransformer's function application operator
     *
     * @param src    Constant reference to a source data structure.
     * @param dst    Reference to a data structure of the same type, src is
     *               copied to.
     *
     * @return void
     */
    void
    operator()(const T &src, T &dst) {
        dst = src;
    };
};

/**
 * Three way container class, similar to StagedContainer
 */
template <typename T>
class StagedVariantContainer : public StagedContainer<T> {
public:
    /**
     * The default constructor uses TrivialTransformer to initialise the
     * structure's transform member. If this is actually done, you should
     * probably be using StagedContainer instead.
     */
    StagedVariantContainer() : transform(TrivialTransformer<T>()){};
    /**
     * The parametrised constructor for StagedVariantContainer
     *
     * @param tf   Transformed used to build the actualArea of the instance.
     */
    StagedVariantContainer(std::function<void(const T &, T &)> tf)
        : transform(tf){};
    /** Accessor for the instance's actualArea */
    const T &
    actual(void) const {
        return actualArea;
    };
    /** Commit stagedArea to activeArea and update actualArea via the
     *  instance's transformer. */
    void
    commit(void) {
        StagedContainer<T>::commit();
        transform(StagedContainer<T>::active(), actualArea);
    };

private:
    std::function<void(const T &, T &)> transform;
    T actualArea;
};
};

#endif /* INC_STAGEDVARIANTCONTAINER_H */
