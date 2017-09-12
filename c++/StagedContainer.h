/*
 * Copyright (c) 2017 micro framework workers, All rights reserved.
 *
 * Terms for redistribution and use can be found in LICENCE.
 */

/**
 * @file StagedContainer.h
 * @brief A generic two-way container class
 *
 * This class implements a double-buffered container capable of storing
 * arbitrary data. The idea is similar to that of a staging area in version
 * control systems: You prepare a set of data, that's not active in any way
 * until you commit that staged data set to be the new active data set.
 *
 * @code
 * MicroFrameWork::StagedContainer<int> foo;
 * foo.staged() = 23;
 * foo.commit();
 * int bar = foo.active();   // bar is now 23.
 * foo.staged() = 42;
 * bar = foo.active();       // bar is still 23.
 * foo.commit();
 * bar = foo.active();       // bar is now 42.
 * @endcode
 */


#ifndef INC_STAGEDCONTAINER_H
#define INC_STAGEDCONTAINER_H

namespace MicroFrameWork {

template <typename T>
class StagedContainer {
public:
    T &
    staged(void) {
        return stagingArea;
    };
    const T &
    active(void) {
        return activeArea;
    };
    void
    commit(void) {
        activeArea = stagingArea;
    };

private:
    T stagingArea;
    T activeArea;
};
};

#endif /* INC_STAGEDCONTAINER_H */
