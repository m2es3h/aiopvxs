#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/version.h>

namespace py = pybind11;

void create_submodule_client(py::module_&);
void create_submodule_data(py::module_&);
void create_submodule_nt(py::module_&);


PYBIND11_MODULE(aiopvxs, m) {
    using namespace pvxs;

    m.doc() = "Python asyncio API to the PVXS libraries";

    m.def("get_version_str", &version_str, "PVXS library version as a string");
    m.def("get_version_int", &version_int, "PVXS_VERSION version");
    m.def("get_version_abi_int", &version_abi_int, "PVXS_ABI_VERSION version");

    py::module_ client = m.def_submodule("client");
    create_submodule_client(client);
    py::module_ data = m.def_submodule("data");
    create_submodule_data(data);
    py::module_ nt = m.def_submodule("nt");
    create_submodule_nt(nt);

}
