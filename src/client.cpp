#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/client.h>

namespace py = pybind11;

inline std::function<void(pvxs::client::Result&&)>
pvxs_result_handler(py::object loop, py::object py_future) {
    return [loop, py_future](pvxs::client::Result&& result) {
        try {
            auto value = pvxs::Value(result());

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
    };
}

inline py::cpp_function
py_future_done_handler(std::shared_ptr<pvxs::client::Operation> op) {
    return py::cpp_function([op](py::object fut) {
        if (fut.attr("cancelled")())
            op->cancel();
    });
}


void create_submodule_client(py::module_& m) {
    using namespace pvxs;
    using namespace pvxs::client;

    py::register_exception<RemoteError>(m, "RemoteError", PyExc_RuntimeError);

    py::class_<Operation, py::smart_holder>(m, "Operation")
        .def("name", &Operation::name)
        .def("cancel", &Operation::cancel);

    py::class_<Context>(m, "Context")
        .def(py::init(&Context::fromEnv))
        .def("close", &Context::close)

        .def("get", [](Context& self, std::string& pv_name) {
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            auto op_builder = self.get(pv_name)
                .result(pvxs_result_handler(loop, py_future));

            auto op = op_builder.exec();
            py_future.attr("add_done_callback")(py_future_done_handler(op));
            return py_future;
        })

        .def("put", [](Context& self, std::string& pv_name, py::dict new_data) {
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            auto op_builder = self.put(pv_name)
                .fetchPresent(true)
                .build([new_data](Value&& current) {
                    Value toput(current.cloneEmpty());

                    py::gil_scoped_acquire lock;
                    py::cast(toput).attr("assign")(new_data);
                    return toput;
                })
                .result(pvxs_result_handler(loop, py_future));

            auto op = op_builder.exec();
            py_future.attr("add_done_callback")(py_future_done_handler(op));
            return py_future;
        }, py::keep_alive<0, 3>());

}
