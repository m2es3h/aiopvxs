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

/*
 * AsyncSubscription
 *
 * Class that pairs a pvxs::client::Subscription with an asyncio.Queue. Allows
 * calling same methods as a Subscription, but with additional methods to await
 * on asyncio.Queue.get() to suspend execution until new data is ready.
 *
 */
class AsyncSubscription {
public:
    AsyncSubscription(std::shared_ptr<pvxs::client::Subscription> sub,
                      py::object py_queue)
        : sub(sub), py_queue(py_queue) {}

    //~AsyncSubscription() { sub->cancel(); }

    bool cancel() { return sub->cancel(); }
    void pause()  { return sub->pause(true); }
    void resume() { return sub->pause(false); }

    const std::string name() { return sub->name(); }

    py::object pop() {
        py::object val;

        try {
            auto val = sub->pop();
            if (val)
                py_queue.attr("put_nowait")(py::cast(val));
        }
        catch (pvxs::client::Finished& fin) {
            py_queue.attr("put_nowait")(py::cast(fin));
        }
        catch (pvxs::client::Connected& con) {
            py_queue.attr("put_nowait")(py::cast(con));
        }
        catch (pvxs::client::Disconnect& dis) {
            py_queue.attr("put_nowait")(py::cast(dis));
        }
        catch (pvxs::client::RemoteError& err) {
            py_queue.attr("put_nowait")(py::cast(err));
        }
        catch (std::exception& exc) {
            py::print("C++ exception thrown in monitor callback:", exc.what());
            py_queue.attr("put_nowait")(py::cast(exc));
        }

        return py_queue.attr("get")();
    }

    py::object get() {
        // return asyncio.Queue.get() co-routine
        return this->pop();
    }

private:
    std::shared_ptr<pvxs::client::Subscription> sub;
    py::object py_queue;
};


void create_submodule_client(py::module_& m) {
    m.doc() = "PVAccess Client API";

    using namespace pvxs;
    using namespace pvxs::client;

    py::register_exception<RemoteError>(m, "RemoteError", PyExc_RuntimeError);
    py::register_exception<Connected>(m, "Connected", PyExc_RuntimeError);
    py::register_exception<Disconnect>(m, "Disconnected", PyExc_RuntimeError);
    py::register_exception<Finished>(m, "Finished", PyExc_RuntimeError);

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

    py::class_<AsyncSubscription, py::smart_holder>(m, "Subscription", "Represents the active event subscription")
        .def("name", &AsyncSubscription::name, "Operation name")
        .def("cancel", &AsyncSubscription::cancel, "Cancels an active event subscription")
        .def("pop", &AsyncSubscription::pop, "Get updated Value from subscription queue")
        .def("get", &AsyncSubscription::get, "Get updated Value from subscription queue (alias for pop())")
        // implement iterator protocol
        .def("__aiter__", [](const AsyncSubscription& self) { return self; })
        .def("__anext__", [](AsyncSubscription& self) {
            // pop() items until some exit condition, then throw StopIteration
            auto val = self.pop();
            //if (!val)
            //    throw py::stop_async_iteration();

            return val;
        });

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

        .def("monitor", [](Context& self, std::string& pv_name) {
            // the result of this method is an aiopvxs.client.Subscription
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_queue = py::module_::import("asyncio").attr("Queue")();

            // make a MonitorBuilder
            auto op_builder = self.monitor(pv_name)
                .event([loop, py_queue](Subscription& sub) {
                    // GIL lock not automatically held in C++ callback,
                    // acquire GIL lock when adding to event loop
                    py::gil_scoped_acquire lock;
                    py::object val;

                    try {
                        // there is always something available if this callback
                        // was called, get it or trigger exception
                        val = py::cast(sub.pop());
                    }
                    catch (Finished& fin) { val = py::cast(fin); }
                    catch (Connected& con) { val = py::cast(con); }
                    catch (Disconnect& dis) { val = py::cast(dis); }
                    catch (RemoteError& rem) { val = py::cast(rem); }
                    catch (std::exception& exc) {
                        py::print("C++ exception thrown in monitor callback:", exc.what());
                        val = py::cast(exc);
                    }
                    // put new data into python asyncio.Queue, unblocking any code
                    // awaiting on new data
                    py::object run_coro_ts = py::module_::import("asyncio").attr("run_coroutine_threadsafe");
                    auto fut = run_coro_ts(py_queue.attr("put")(val), loop);
                    fut.attr("result")();

                    /*py_queue.attr("put_nowait")(val);
                    py::print("done CALLBACK");
                    loop.attr("call_soon_threadsafe")(
                        py::cpp_function([py_queue, upd = std::move(val)]() {
                            py::print("in CALLBACK");
                            py_queue.attr("put_nowait")(upd);
                            py::print("done CALLBACK");
                        })
                    );*/

                    return;
                });

            // start the subscription operation
            auto sub = op_builder.exec();
            // attach asyncio.Event to the Subscription that is set by monitor event callback
            auto sub_with_event = AsyncSubscription(sub, py_queue);
            // return the subscription
            return sub_with_event;
        }, "Constructs a MonitorBuilder for the operation and executes it, returning "
           "an aiopvxs.client.Subscription object that can be iterated with an async "
           "for loop or cancelled.");
}
