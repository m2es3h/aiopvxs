/*
 * Project: aiopvxs
 * File:    client.cpp
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

#include <pybind11/pybind11.h>
#include <pybind11/native_enum.h>
#include <pybind11/functional.h>
#include <pybind11/stl.h>

#include <pvxs/client.h>

namespace py = pybind11;

/*
 * pvxs_result_handler
 *
 * Returns a std::function<> that can be used as client Context
 * result handler. Assigns value or exception to the asyncio.Future
 * that represents the transaction in progress.
 *
 * The result is not directly assigned, rather a callback on the event
 * loop is scheduled to run very soon. This is the thread-safe way to
 * synchronize C++ events with Python asyncio events.
 *
 */
inline std::function<void(pvxs::client::Result&&)>
pvxs_result_handler(py::object loop, py::object py_future) {
    // lambda capture copies of asyncio event loop and Future
    return [loop, py_future](pvxs::client::Result&& result) {
        // GIL lock not automatically held in C++ callback, acquire GIL lock
        py::gil_scoped_acquire lock;
        try {
            // test result for value or exception
            pvxs::Value value = pvxs::Value(result());
            // if value, schedule asyncio.Future.set_result(value)
            // call on the event loop
            loop.attr("call_soon_threadsafe")(
                // GIL lock is held by default when py::cpp_function
                // eventually executes
                py::cpp_function([py_future, value]() {
                    py_future.attr("set_result")(value);
                })
            );
        }
        // if exception, schedule asyncio.Future.set_exception(py_exc)
        // call on the event loop
        catch (const py::key_error& e) {
            py::object py_exc = py::module_::import("builtins").attr("KeyError")(e.what());

            loop.attr("call_soon_threadsafe")(
                py::cpp_function([py_future, py_exc]() {
                    py_future.attr("set_exception")(py_exc);
                })
            );
        }
        catch (const py::type_error& e) {
            py::object py_exc = py::module_::import("builtins").attr("TypeError")(e.what());

            loop.attr("call_soon_threadsafe")(
                py::cpp_function([py_future, py_exc]() {
                    py_future.attr("set_exception")(py_exc);
                })
            );
        }
        catch (const std::exception& e) {
            py::object py_exc = py::module_::import("builtins").attr("ValueError")(e.what());

            loop.attr("call_soon_threadsafe")(
                py::cpp_function([py_future, py_exc]() {
                    py_future.attr("set_exception")(py_exc);
                })
            );
        }
    };
}

/*
 * py_future_done_handler
 *
 * Returns a std::function<> that can be used as an asyncio.Future
 * done callback. By capturing value of pvxs::client::Operation here
 * in the returned lambda function, this done handler maintains a
 * reference to the Operation until after the Operation is complete.
 *
 */
template <typename T>
inline py::cpp_function
py_future_done_handler(std::shared_ptr<T> op) {
   static_assert(std::is_same<T, pvxs::client::Operation>::value ||
                 std::is_same<T, pvxs::client::Subscription>::value,
                "Only Operation and Subscription are supported");

    // the lambda capture here is keeping the operation alive while it runs
    return py::cpp_function([op](py::object fut) {
        // if Future was cancelled, also call Operation::cancel()
        if (fut.attr("cancelled")())
            op->cancel();
    });
}


void create_submodule_client(py::module_& m) {
    m.doc() = "PVAccess Client API";

    using namespace pvxs;
    using namespace pvxs::client;

    py::register_exception<RemoteError>(m, "RemoteError", PyExc_RuntimeError);

    py::native_enum<Discovered::event_t>(m, "EventTypeEnum", "enum.IntEnum")
        .value("Online", Discovered::event_t::Online)
        .value("Timeout", Discovered::event_t::Timeout)
        .finalize();

    py::class_<Discovered>(m, "Discovered", "")
        .def_readonly("event", &Discovered::event)
        .def_readonly("peerVersion", &Discovered::peerVersion)
        .def_readonly("peer", &Discovered::peer)
        .def_readonly("proto", &Discovered::proto)
        .def_readonly("server", &Discovered::server);

    // Operations are always wrapped in a shared_ptr<>, define py::smart_holder
    // here to auto-matically manage that
    py::class_<Operation, py::smart_holder>(m, "Operation", "Represents the in-progress network transaction")
        .def("name", &Operation::name, "Operation name")
        .def("cancel", &Operation::cancel, "Cancels a in-progress network transaction");

    py::class_<Subscription, py::smart_holder>(m, "Subscription", "Represents the active event subscription")
        .def("name", &Subscription::name, "Operation name")
        .def("cancel", &Subscription::cancel, "Cancels an active event subscription");

    py::class_<Context>(m, "Context", "PVAccess protocol client")
        .def(py::init(&Context::fromEnv), "Initialise a Context with settings from Config::fromEnv()")
        .def("close", &Context::close, "Disconnects any active clients and closes network connection")

        .def("get", [](Context& self, std::string& pv_name) {
            // the result of this method is an asyncio.Future, so get() can be
            // treated like a co-routine (must await get(...) to retrieve the result)
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            // make a GetBuilder with result callback that assigns the result of the
            // operation to an asyncio.Future (using either set_result() or set_exception())
            auto op_builder = self.get(pv_name)
                .result(pvxs_result_handler(loop, py_future));

            // start the operation
            auto op = op_builder.exec();
            // attach done handler to the asyncio.Future so the operation continues until completion
            py_future.attr("add_done_callback")(py_future_done_handler(op));
            // return asyncio.Future representing the future result of the operation
            return py_future;
        }, "Constructs a GetBuilder for the operation and executes it, returning "
           "an asyncio.Future representing the future result of the operation")

        .def("put", [](Context& self, std::string& pv_name, py::dict new_data) {
            // the result of this method is an asyncio.Future, so put() can be
            // treated like a co-routine (must await put(...) to retrieve the result)
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            // make a PutBuilder with result callback that assigns the result of the
            // operation to an asyncio.Future (using either set_result() or set_exception())
            auto op_builder = self.put(pv_name)
                .fetchPresent(true)
                .build([new_data](Value&& current) {
                    // after initial get operation, apply python new_data to the
                    // pvxs::Value to take advantage of the automatic type casting
                    Value toput(current.cloneEmpty());

                    // GIL lock not automatically held in C++ callback, acquire GIL lock
                    py::gil_scoped_acquire lock;
                    try {
                        // new_data is a python dictionary, assign it
                        // to recursively cast each key to its field
                        py::cast(toput).attr("assign")(new_data);
                    }
                    catch (py::error_already_set& e) {
                        // if any python exceptions are raised, need to catch them all
                        // here and turn them into C++ exceptions so they can pass to
                        // the C++ result handler without invoking Python interpreter's
                        // error handling code
                        if (e.matches(PyExc_KeyError))
                            throw py::key_error(e.what());
                        else if (e.matches(PyExc_TypeError))
                            throw py::type_error(e.what());
                        else
                            throw py::value_error(e.what());
                    }
                    return toput;
                })
                .result(pvxs_result_handler(loop, py_future));

            // start the operation
            auto op = op_builder.exec();
            // attach done handler to the asyncio.Future so the operation continues until completion
            py_future.attr("add_done_callback")(py_future_done_handler(op));
            // return asyncio.Future representing the future result of the operation
            return py_future;
        // the py::keep_alive means the 3rd argument (py::dict new_data) must live at least as long
        // as the return value, otherwise new_data might get cleaned up before .build() callback
        }, py::keep_alive<0, 3>(), "Constructs a PutBuilder for the operation and executes it, returning "
                                   "an asyncio.Future representing the future result of the operation")

        .def("discover", [](Context& self, std::function<void(const Discovered&)> cb, bool do_ping) {
            // the result of this method is an asyncio.Future,
            // await discover(...) with a timeout
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            // make a DiscoverBuilder
            // callback "cb" is actually a temporary std::function created by pybind11
            // that can be moved
            auto op_builder = self.discover(std::move(cb))
                .pingAll(do_ping);

            // start the operation
            auto op = op_builder.exec();
            // attach done handler to the asyncio.Future (to call op.cancel() when done)
            py_future.attr("add_done_callback")(py_future_done_handler(op));
            // return asyncio.Future, can await with timeout or call .cancel() on it
            return py_future;
        }, "Constructs a DiscoverBuilder for the operation and executes it, returning "
           "an asyncio.Future that can be awaited (with a timeout) or cancelled. It will "
           "never return a result, rather the discover results will arrive via the provided "
           "callback function.")

        .def("monitor", [](Context& self, std::string& pv_name, py::object py_queue) {
            // the result of this method is an asyncio.Future,
            // await discover(...) with a timeout
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            // make a MonitorBuilder
            auto op_builder = self.monitor(pv_name)
                .event([py_queue](Subscription& sub) {
                    // GIL lock not automatically held in C++ callback,
                    // acquire GIL lock when adding to python Queue
                    try {
                        Value val_update = sub.pop();
                        if (val_update) {
                            py::gil_scoped_acquire lock;
                            py_queue.attr("put_nowait")(val_update);
                        }
                    }
                    catch (std::exception& e) {
                        py::gil_scoped_acquire lock;
                        py_queue.attr("put_nowait")(e);
                    }
                });

            // start the operation
            auto op = op_builder.exec();
            // attach done handler to the asyncio.Future (to call op.cancel() when done)
            py_future.attr("add_done_callback")(py_future_done_handler(op));
            // return asyncio.Future, can await with timeout or call .cancel() on it
            return py_future;
        }, "Constructs a MonitorBuilder for the operation and executes it, returning "
           "an asyncio.Future that can be awaited (with a timeout) or cancelled. It will "
           "never return a result, rather the monitor results will be put in the "
           "provided asyncio.Queue.");
}
