#include <iostream>
#include <chrono>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/client.h>

namespace py = pybind11;


class AwaitableOperation {
public:
    AwaitableOperation(std::shared_ptr<pvxs::client::Operation> op,
                       py::object py_future) : 
        op(op), py_future(py_future) {}

    std::string name() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return std::string(op->name());
    }

    bool cancel() {
        bool cancelled = op->cancel();
        if (cancelled)
            py_future.attr("cancel")();
        return cancelled;
    }

    py::object wait() {
        return py_future.attr("result")();
    }

    py::object await() {
        return py_future.attr("__await__")();
    }

private:
    std::shared_ptr<pvxs::client::Operation> op;
    py::object py_future;
};


void create_submodule_client(py::module_& m) {
    using namespace pvxs;
    using namespace pvxs::client;

    py::class_<AwaitableOperation, py::smart_holder>(m, "AwaitableOperation")
        .def("name", &AwaitableOperation::name)
        .def("cancel", &AwaitableOperation::cancel)
        .def("wait", &AwaitableOperation::wait)
        .def("__await__", &AwaitableOperation::await);

    py::class_<Context, py::smart_holder>(m, "Context")
        .def(py::init(&Context::fromEnv))
        .def("close", &Context::close)
        .def("get", [](Context& self, std::string& pv_name) {
            py::object loop = py::module_::import("asyncio").attr("get_event_loop")();
            py::object py_future = loop.attr("create_future")();

            auto op_builder = self.get(pv_name);
            op_builder.result([loop, py_future](Result&& result) {
                std::cout << "!DONE! getting result" << std::endl;

                try {
                    result();
                } catch(const std::exception& e) {
                    std::cout << "EXCEPTION: " << e.what() << std::endl;
                }

                auto reply = result();

                std::cout << "!DONE! scheduling callback" << std::endl;
                py::gil_scoped_acquire lock;
                std::cout << "!DONE! got lock" << std::endl;
                loop.attr("call_soon_threadsafe")(
                    py::cpp_function([py_future, reply]() {
                        std::cout << "!cb! callback started" << std::endl;
                        //py::gil_scoped_acquire lock;
                        std::cout << "!cb! got lock" << std::endl;
                        py_future.attr("set_result")(reply);
                        std::cout << "!cb! callback finished" << std::endl;
                    })
                );

            });

            std::shared_ptr<Operation> op = op_builder.exec();
            return AwaitableOperation(op, py_future);
        });

}
