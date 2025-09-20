#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/nt.h>

namespace py = pybind11;

void create_submodule_nt(py::module_& m) {
    m.doc() = "Normative Type definitions";

    using namespace pvxs::nt;

    py::class_<pvxs::nt::NTScalar>(m, "NTScalar")
        .def(py::init<pvxs::TypeCode::code_t>())
        .def("create", &pvxs::nt::NTScalar::create);

    py::class_<pvxs::nt::NTEnum>(m, "NTEnum")
        .def(py::init<>())
        .def("create", &pvxs::nt::NTEnum::create);

}
