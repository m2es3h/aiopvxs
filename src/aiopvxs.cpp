#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/version.h>

namespace py = pybind11;

PYBIND11_MODULE(aiopvxs, m) {
    using namespace pvxs;

    m.doc() = "Python asyncio API to the PVXS libraries";

    m.def("get_version_str", &version_str, "PVXS library version as a string");
    m.def("get_version_int", &version_int, "PVXS_VERSION version");
    m.def("get_version_abi_int", &version_abi_int, "PVXS_ABI_VERSION version");

}
