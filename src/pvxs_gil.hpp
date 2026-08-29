/*
 * Project: aiopvxs
 * File:    pvxs_gil.hpp
 *
 * This file is part of aiopvxs.
 *
 * https://github.com/m2es3h/aiopvxs
 *
 * Copyright (C) Michael Smith. All rights reserved.
 *
 * aiopvxs is free software: you can redistribute it and/or modify it
 * under the terms of The 3-Clause BSD License.
 *
 * https://opensource.org/license/bsd-3-clause
 *
 * aiopvxs is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef AIOPVXS_GIL_HPP
#define AIOPVXS_GIL_HPP

#include <memory>
#include <utility>

/*
 * Helpers for the Python GIL that keep Python and C++ from deadlocking each other
 *
 * Much of the pvxs API (cancel, close, start, stop, and the implicit cancel when
 * the last reference to an Operation or Subscription is released) hands the work
 * to pvxs' event loop works and waits on it. Every aiopvxs callback runs on one
 * of those same workers and needs the GIL, so aiopvxs must never wait on a pvxs
 * worker while holding the GIL.
 *
 * Additionally, pvxs callbacks take lambdas that capture Python objects by value
 * (asyncio loop, Queue). When the callback is finished, a pvxs worker thread
 * release these Python resources. The refernce count of these Python objects
 * need to be decremented while holding the GIL.
 * 
 */

#include <pybind11/pybind11.h>

 /*
  * pvxs_without_gil()
  *
  * Runs a function with the GIL released when the current thread already holds the GIL.
  * Safe to call from a destructor, can run in a Python thread or pvxs worker thread.
  */
template <typename Fn, typename... Args>
inline auto pvxs_without_gil(Fn&& fn, Args&&... args) {
    if (PyGILState_Check()) {
        pybind11::gil_scoped_release release;
        return std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
    else {
        return std::forward<Fn>(fn)(std::forward<Args>(args)...);
    }
}

#endif // AIOPVXS_GIL_HPP
