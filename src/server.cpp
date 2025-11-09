#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/server.h>
#include <pvxs/sharedpv.h>

namespace py = pybind11;


void create_submodule_server(py::module_& m) {
    using namespace pvxs;
    using namespace pvxs::server;

    py::class_<StaticSource>(m, "StaticSource")
        .def(py::init(&StaticSource::build))
        .def("add", &StaticSource::add)
        .def("remove", &StaticSource::remove)
        .def("list", &StaticSource::list)
        .def(py::init([](const std::map<std::string, SharedPV>& provider) {
            auto src = StaticSource::build();
            for (const auto& pv : provider)
                src.add(pv.first, pv.second);
            return src;
        }));

    py::class_<SharedPV>(m, "SharedPV")
        .def(py::init(&SharedPV::buildMailbox))
        .def("open", &SharedPV::open)
        .def("close", &SharedPV::close)
        .def(py::init([](const TypeDef& nt, Value initial) {
            auto init_value = nt.create().assign(initial);
            auto pv = SharedPV::buildMailbox();
            pv.open(init_value);
            return pv;
        }), py::arg("nt"), py::arg("initial"));

    py::class_<Server>(m, "Server")
        .def("listSource", &Server::listSource)
        .def("start", &Server::start)
        .def("stop", &Server::stop)
        .def("run", &Server::run)
        .def("interrupt", &Server::interrupt)
        .def("__enter__", [](Server& self) {
            self.start();
        })
        .def("__exit__", [](Server& self, py::object exc_type,
                                          py::object exc_value,
                                          py::object traceback) {
            self.stop();
            //if (exc_type.is(py::none())) {
            //    std::cout << "no exceptions" << std::endl;
            //}
            //else {
            //    std::cout << "exception raised" << std::endl;
            ///}
        })
        .def(py::init(&Server::fromEnv))
        .def(py::init([](const std::map<std::string, SharedPV>& provider) {
            auto src = StaticSource::build();
            for (const auto& pv : provider)
                src.add(pv.first, pv.second);

            auto server = Server::fromEnv();
            server.addSource("StaticSource", src.source());
            return server;
        }), py::arg("provider"));

}
