/*
 * Project: aiopvxs
 * File:    aiopvxs.cpp
 *
 * This file is part of aiopvxs.
 *
 * https://github.com/m2es3h/aiopvxs
 *
 * Copyright (C) Michael Smith. All rights reserved.
 *
 * aiopvxs is free software: you can redistribute it and/or modify it
 * under the terms of The 3-Clause BSD License.
 *
 * https://opensource.org/license/bsd-3-clause
 *
 * aiopvxs is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/version.h>

namespace py = pybind11;

void create_submodule_client(py::module_&);
void create_submodule_data(py::module_&);
void create_submodule_nt(py::module_&);
void create_submodule_server(py::module_&);


PYBIND11_MODULE(aiopvxs, m) {
    using namespace pvxs;

    m.doc() = "Python asyncio API to the PVXS libraries";

    m.def("get_version_str", &version_str, "PVXS library version as a string");
    m.def("get_version_int", &version_int, "PVXS_VERSION version");
    m.def("get_version_abi_int", &version_abi_int, "PVXS_ABI_VERSION version");

    // from aiopvxs.client import ...
    py::module_ client = m.def_submodule("client");
    create_submodule_client(client);
    // from aiopvxs.data import ...
    py::module_ data = m.def_submodule("data");
    create_submodule_data(data);
    // from aiopvxs.nt import ...
    py::module_ nt = m.def_submodule("nt");
    create_submodule_nt(nt);
    // from aiopvxs.server import ...
    py::module_ server = m.def_submodule("server");
    create_submodule_server(server);

}
