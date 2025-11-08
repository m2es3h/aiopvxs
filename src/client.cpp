#include <chrono>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/client.h>

namespace py = pybind11;


void create_submodule_client(py::module_& m) {
    using namespace pvxs;
    using namespace pvxs::client;

    py::register_exception<RemoteError>(m, "RemoteError", PyExc_RuntimeError);

    py::class_<Operation, py::smart_holder>(m, "Operation")
        .def("name", &Operation::name)
        .def("cancel", &Operation::cancel);

    py::class_<Context, py::smart_holder>(m, "Context")
        .def(py::init(&Context::fromEnv))
        .def("close", &Context::close)

        .def("get", [](Context& self, std::string& pv_name) {
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            auto op_builder = self.get(pv_name)
                .result([loop, py_future](Result&& result) {
                    try {
                        auto value = Value(result());

                        py::gil_scoped_acquire lock;
                        loop.attr("call_soon_threadsafe")(
                            py::cpp_function([py_future, value]() {
                                py_future.attr("set_result")(value);
                            })
                        );
                    }
                    catch (const std::runtime_error& exc_info) {
                        py::gil_scoped_acquire lock;
                        loop.attr("call_soon_threadsafe")(
                            py::cpp_function([py_future, exc_info]() {
                                py::object py_exc_type = py::module_::import("aiopvxs").attr("client");
                                py::object py_exc = py_exc_type.attr("RemoteError")(exc_info.what());
                                py_future.attr("set_exception")(py_exc);
                            })
                        );
                    }
                });

            auto op = op_builder.exec();
            py_future.attr("add_done_callback")(
                py::cpp_function([op](py::object py_future) {
                    if (py_future.attr("cancelled")())
                        op->cancel();
                })
            );
            return py_future;
        })

        .def("put", [](Context& self, std::string& pv_name, py::dict new_data) {
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            auto op_builder = self.put(pv_name);
            /*
            for (auto item : new_data) {
                const std::string key = item.first.cast<std::string>();
                const py::object& py_value = py::reinterpret_borrow<py::object>(item.second);
                if (py::isinstance<py::int_>(py_value))
                    op_builder.set(key, item.second.cast<int64_t>());
                else if (py::isinstance<py::float_>(py_value))
                    op_builder.set(key, item.second.cast<double>());
                else if (py::isinstance<py::str>(py_value))
                    op_builder.set(key, item.second.cast<std::string>());
                else if (py::isinstance<py::sequence>(py_value) ||
                         py::isinstance<py::buffer>(py_value))
                    op_builder.set(key, item.second.cast<shared_array<const void>>());
            }*/
            op_builder.fetchPresent(true);
            op_builder.build([new_data](Value&& current) {
                Value toput(current.cloneEmpty());

                py::gil_scoped_acquire lock;
                py::cast(toput).attr("assign")(new_data);
                return toput;
            });
            op_builder.result([loop, py_future](Result&& result) {
                try {
                    auto value = Value(result());

                    py::gil_scoped_acquire lock;
                    loop.attr("call_soon_threadsafe")(
                        py::cpp_function([py_future, value]() {
                            py_future.attr("set_result")(value);
                        })
                    );
                }
                catch (const std::runtime_error& exc_info) {
                    py::gil_scoped_acquire lock;
                    loop.attr("call_soon_threadsafe")(
                        py::cpp_function([py_future, exc_info]() {
                            py::object py_exc_type = py::module_::import("aiopvxs").attr("client");
                            py::object py_exc = py_exc_type.attr("RemoteError")(exc_info.what());
                            py_future.attr("set_exception")(py_exc);
                        })
                    );
                }
            });

            auto op = op_builder.exec();
            py_future.attr("add_done_callback")(
                py::cpp_function([op](py::object py_future) {
                    if (py_future.attr("cancelled")())
                        op->cancel();
                })
            );
            return py_future;
        }, py::keep_alive<0, 3>());

}
