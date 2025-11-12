#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/server.h>
#include <pvxs/sharedpv.h>

namespace py = pybind11;


void create_submodule_server(py::module_& m) {
    m.doc() = "PVAccess Server API";

    using namespace pvxs;
    using namespace pvxs::server;

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
            auto init_value = nt.create().assign(initial);
            auto pv = SharedPV::buildMailbox();
            pv.open(init_value);
            return pv;
        }), py::arg("nt"), py::arg("initial"), "Provide data type and initial value to SharedPV")
 
        // class methods
        .def("open", &SharedPV::open, "Infer data type from initial value to SharedPV")
        .def("close", &SharedPV::close, "Disconnects any active clients of SharedPV");

    py::class_<Server>(m, "Server", "PVAccess protocol server")

        // constructors
        .def(py::init(&Server::fromEnv), "Initialise a Server with settings from Config::fromEnv()")
        .def(py::init([](const std::map<std::string, SharedPV>& provider) {
            auto src = StaticSource::build();
            for (const auto& pv : provider)
                src.add(pv.first, pv.second);

            auto server = Server::fromEnv();
            server.addSource("StaticSource", src.source());
            return server;
        }), py::arg("provider"), "Initialize a Server with dictionary of SharedPVs")

        // class methods
        .def("listSource", &Server::listSource, "Return list[tuple] with source names and priority ranking")
        .def("start", &Server::start, "Start the Server")
        .def("stop", &Server::stop, "Stop the Server")
        .def("run", &Server::run, "Start the Server and block execution")
        .def("interrupt", &Server::interrupt, "Queue a request to unblock run()")

        // python helper methods
        // implement a context manager protocol so users can run server using 'with' statement
        // see pvxs_test_server() pytest fixture in src/tests/conftest.py for example usage
        .def("__enter__", [](Server& self) {
            self.start();
            return self;
        })
        .def("__exit__", [](Server& self, py::object exc_type,
                                          py::object exc_value,
                                          py::object traceback) {
            self.stop();
            // uncaught exceptions within the context manager are available
            //if (exc_type.is(py::none())) {
            //    std::cout << "no exceptions" << std::endl;
            //}
            //else {
            //    std::cout << "exception raised" << std::endl;
            ///}
        });

}
