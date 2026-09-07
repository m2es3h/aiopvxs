/*
 * Project: aiopvxs
 * File:    server.cpp
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
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <pvxs/server.h>
#include <pvxs/sharedpv.h>

#include "pvxs_gil.hpp"

namespace py = pybind11;

/*
 * python_op_handler
 *
 * Wraps a python SharedPV callback so that an exception within the
 * callback is reported to the caller with ExecOp::error().
 *
 */
inline std::function<void(pvxs::server::SharedPV&,
                          std::unique_ptr<pvxs::server::ExecOp>&&,
                          pvxs::Value&&)>
python_op_handler(py::function py_fn, std::string source_hint) {
    // reference to callback function will be destructed by a pvxs worker thread
    // ensure that the Python objects are destructed while holding the GIL
    auto callback_ref = pvxs_call_cpp_dtor_with_gil(py_fn);

    return [callback_ref, source_hint](pvxs::server::SharedPV& pv,
                                       std::unique_ptr<pvxs::server::ExecOp>&& op,
                                       pvxs::Value&& val) {
        // GIL lock not automatically held in C++ callback, acquire
        py::gil_scoped_acquire lock;
        // transfer ownership of ExecOp to a py::object (callback should not
        // consume the op, might need to call op.error() after py_fn() call)
        py::object py_op = py::cast(std::move(op));

        try {
            // run the python callback -> py_fn(pv, op, value)
            py::object callback = *callback_ref;
            callback(py::cast(pv), py_op, py::cast(std::move(val)));
        }
        catch (py::error_already_set& e) {
            // if a python exception escapes the callback, set the
            // op.error() so the client gets the exception message
            try {
                std::stringstream ss;
                ss << (e.type() ? py::str(e.type().attr("__name__")).cast<std::string>()
                                : "Exception");
                ss << ": ";
                ss << (e.value() ? py::str(e.value()).cast<std::string>()
                                 : e.what());
                py_op.cast<pvxs::server::ExecOp&>().error(ss.str());
            }
            catch (...) {  // let no further exceptions escape
                // report the full traceback via sys.unraisablehook
                e.discard_as_unraisable(source_hint.c_str());
            }
        }
    };
}


void create_submodule_server(py::module_& m) {
    m.doc() = "PVAccess Server API";

    using namespace pvxs;
    using namespace pvxs::server;

    py::class_<ExecOp>(m, "ExecOp", "Handle for server-side operation on a PV.")
        .def("reply", static_cast<void (ExecOp::*)()>(&ExecOp::reply), "Issue a reply without data")
        .def("reply", static_cast<void (ExecOp::*)(const Value&)>(&ExecOp::reply), "Issue a reply with data")
        .def("error", &ExecOp::error, "Indicate the request has resulted in an error");

    py::class_<StaticSource>(m, "StaticSource", "Associate SharedPV instances with a name")

        // constructors
        .def(py::init(&StaticSource::build), "Initialise empty StaticSource")
        .def(py::init([](const std::map<std::string, SharedPV>& provider) {
            auto src = StaticSource::build();
            for (const auto& pv : provider)
                src.add(pv.first, pv.second);
            return src;
        }), "Initialise StaticSource with a dictionary of {'name': SharedPV}")

        // class methods
        .def("add", &StaticSource::add, "Add SharedPV by name")
        .def("remove", &StaticSource::remove, "Remove SharedPV by name")
        .def("list", &StaticSource::list, "Returns dictionary of {'name': SharedPV}");

    py::class_<SharedPV>(m, "SharedPV", "Process variable (PV) data that can be accessed via Server")

        // constructors
        .def(py::init(&SharedPV::buildMailbox), "Initialise writable SharedPV")
        .def(py::init([](const TypeDef& nt, Value initial) {
            auto init_value = nt.create();
            init_value.assign(initial);

            auto pv = SharedPV::buildMailbox();
            pv.open(initial);
            return pv;
        }), py::arg("nt"), py::arg("initial"), "Provide data type and initialise SharedPV from Value")
        .def(py::init([](const TypeDef& nt, py::dict initial) {
            auto init_value = py::cast(nt.create());
            init_value.attr("assign")(initial);

            auto pv = SharedPV::buildMailbox();
            pv.open(init_value.cast<Value&>());
            return pv;
        }), py::arg("nt"), py::arg("initial"), "Provide data type and initialise SharedPV from python dictionary")
 
        // class methods
        .def("open", &SharedPV::open, "Infer data type from initial value to SharedPV")
        .def("close", &SharedPV::close, "Disconnects any active clients of SharedPV")
        .def("post", &SharedPV::post, "Update the cached value of SharedPV")

        //.def("onPut", &SharedPV::onPut)
        .def("onPut", [](SharedPV& self, py::function py_fn) {
            self.onPut(python_op_handler(py_fn, "aiopvxs SharedPV onPut callback"));
        }, "Install a custom callback function for PUT operations on this PV.")
        //.def("onRPC", &SharedPV::onRPC);
        .def("onRPC", [](SharedPV& self, py::function py_fn) {
            self.onRPC(python_op_handler(py_fn, "aiopvxs SharedPV onRPC callback"));
        }, "Install a custom callback function for RPC operations on this PV.");

    // Server::Pvt::~Pvt() calls stop(), which wait for pvxs workers, which might
    // be waiting for the GIL, so do not hold GIL on Server destruction
    py::class_<Server>(m, "Server", py::release_gil_before_calling_cpp_dtor(),
                                    "PVAccess protocol server")

        // constructors
        // input arguments are converted before method is called, safe to not hold GIL
        .def(py::init(&Server::fromEnv), py::call_guard<py::gil_scoped_release>(),
                                         "Initialise a Server with settings from Config::fromEnv()")
        .def(py::init([](const std::map<std::string, SharedPV>& provider) {
            auto src = StaticSource::build();
            for (const auto& pv : provider)
                src.add(pv.first, pv.second);

            auto server = Server::fromEnv();
            server.addSource("StaticSource", src.source());
            return server;
        }), py::arg("provider"), py::call_guard<py::gil_scoped_release>(),
            "Initialize a Server with dictionary of SharedPVs")

        // class methods
        // each of these hands work to the server's workers, then those workers
        // run SharedPV callbacks which need the GIL
        .def("listSource", &Server::listSource, py::call_guard<py::gil_scoped_release>(),
                           "Return list[tuple] with source names and priority ranking")
        .def("start", &Server::start, py::call_guard<py::gil_scoped_release>(),
                      "Start the Server")
        .def("stop", &Server::stop, py::call_guard<py::gil_scoped_release>(),
                     "Stop the Server")
        .def("run", &Server::run, py::call_guard<py::gil_scoped_release>(),
                    "Start the Server and block the calling thread until interrupt(), "
                    "or until SIGINT is received (handled by pvxs installed SIGINT handler)")
        .def("interrupt", &Server::interrupt, py::call_guard<py::gil_scoped_release>(),
                          "Queue a request to unblock run()")

        // python helper methods
        // implement a context manager protocol so users can run server using 'with' statement
        // see pvxs_test_server() pytest fixture in src/tests/conftest.py for example usage
        // start()/stop() are called without GIL since it waits on pvxs worker threads
        .def("__enter__", [](Server& self) {
            self.start();
            return self;
        }, py::call_guard<py::gil_scoped_release>())
        .def("__exit__", [](Server& self, const py::object& exc_type,
                                          const py::object& exc_value,
                                          const py::object& traceback) {
            self.stop();
            // uncaught exceptions within the context manager are available
            //if (exc_type.is(py::none())) {
            //    std::cout << "no exceptions" << std::endl;
            //}
            //else {
            //    std::cout << "exception raised" << std::endl;
            ///}
        }, py::call_guard<py::gil_scoped_release>());

}
