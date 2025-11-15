/*
 * Project: aiopvxs
 * File:    nt.cpp
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

#include <pvxs/nt.h>

namespace py = pybind11;


void create_submodule_nt(py::module_& m) {
    m.doc() = "Normative Type definitions";

    using namespace pvxs::nt;

    py::class_<NTScalar>(m, "NTScalar", "EPICS V4 Normative Type that describes a single scalar value")
        .def(py::init<pvxs::TypeCode::code_t>(), "Initialise with TypeCodeEnum")
        .def("build", &NTScalar::build, "Turn TypeCode into TypeDef")
        .def("create", &NTScalar::create, "Turn TypeCode into empty Value");

    py::class_<NTEnum>(m, "NTEnum", "EPICS V4 Normative Type that describes an enumeration")
        .def(py::init<>(), "Default contructor")
        .def("build", &NTEnum::build, "Turn TypeCode into TypeDef")
        .def("create", &NTEnum::create, "Turn TypeCode into empty Value");

}
