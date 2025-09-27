#include <iostream>
#include <chrono>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/client.h>

namespace py = pybind11;


void create_submodule_client(py::module_& m) {
    using namespace pvxs;
    using namespace pvxs::client;

    py::class_<Operation, py::smart_holder>(m, "Operation")
        .def("name", &Operation::name)
        .def("cancel", &Operation::cancel);

    py::class_<Context, py::smart_holder>(m, "Context")
        .def(py::init(&Context::fromEnv))
        .def("close", &Context::close)
        .def("get", [](Context& self, std::string& pv_name) {
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            auto op_builder = self.get(pv_name).result(
                [loop, py_future](Result&& result) {
                    try {
                        auto value = Value(result());

                        py::gil_scoped_acquire lock;
                        loop.attr("call_soon_threadsafe")(
                            py::cpp_function([py_future, value]() {
                                py_future.attr("set_result")(value);
                            })
                        );
                    }
                    catch (const std::exception exc_info) {
                        py::gil_scoped_acquire lock;
                        loop.attr("call_soon_threadsafe")(
                            py::cpp_function([py_future, exc_info]() {
                                py_future.attr("set_exception")(exc_info);
                            })
                        );
                    }
                });

            py_future.attr("pvxs_operation") = op_builder.exec();
            return py_future;
        });

}
