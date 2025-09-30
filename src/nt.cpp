#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/nt.h>

namespace py = pybind11;


void create_submodule_nt(py::module_& m) {
    m.doc() = "Normative Type definitions";

    using namespace pvxs::nt;

    py::class_<NTScalar>(m, "NTScalar")
        .def(py::init<pvxs::TypeCode::code_t>())
        .def("build", &NTScalar::build)
        .def("create", &NTScalar::create);

    py::class_<NTEnum>(m, "NTEnum")
        .def(py::init<>())
        .def("build", &NTEnum::build)
        .def("create", &NTEnum::create);

}
