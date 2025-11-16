/*
 * Project: aiopvxs
 * File:    data.cpp
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
#include <pybind11/native_enum.h>
#include <pybind11/stl.h>

#include <pvxs/data.h>

#include "pvxs_types.hpp"

namespace py = pybind11;


void create_submodule_data(py::module_& m) {
    m.doc() = "Data Type and Value classes";

    using namespace pvxs;

    py::register_exception<NoField>(m, "NoField", PyExc_KeyError);
    py::register_exception<NoConvert>(m, "NoConvert", PyExc_TypeError);
    py::register_exception<LookupError>(m, "LookupError", PyExc_KeyError);

    py::native_enum<TypeCode::code_t>(m, "TypeCodeEnum", "enum.IntEnum")
        .value("Bool", TypeCode::code_t::Bool)
        .value("UInt8", TypeCode::code_t::UInt8)
        .value("UInt8A", TypeCode::code_t::UInt8A)
        .value("UInt16", TypeCode::code_t::UInt16)
        .value("UInt16A", TypeCode::code_t::UInt16A)
        .value("UInt32", TypeCode::code_t::UInt32)
        .value("UInt32A", TypeCode::code_t::UInt32A)
        .value("UInt64", TypeCode::code_t::UInt64)
        .value("UInt64A", TypeCode::code_t::UInt64A)
        .value("Int8", TypeCode::code_t::Int8)
        .value("Int8A", TypeCode::code_t::Int8A)
        .value("Int16", TypeCode::code_t::Int16)
        .value("Int16A", TypeCode::code_t::Int16A)
        .value("Int32", TypeCode::code_t::Int32)
        .value("Int32A", TypeCode::code_t::Int32A)
        .value("Int64", TypeCode::code_t::Int64)
        .value("Int64A", TypeCode::code_t::Int64A)
        .value("Float32", TypeCode::code_t::Float32)
        .value("Float32A", TypeCode::code_t::Float32A)
        .value("Float64", TypeCode::code_t::Float64)
        .value("Float64A", TypeCode::code_t::Float64A)
        .value("String", TypeCode::code_t::String)
        .value("StringA", TypeCode::code_t::StringA)
        .value("Struct", TypeCode::code_t::Struct)
        .value("Null", TypeCode::code_t::Null)
        //.export_values()
        .finalize();

    py::native_enum<StoreType>(m, "StoreTypeEnum", "enum.IntEnum")
        .value("Null", StoreType::Null)
        .value("Bool", StoreType::Bool)
        .value("UInteger", StoreType::UInteger)
        .value("Integer", StoreType::Integer)
        .value("Real", StoreType::Real)
        .value("String", StoreType::String)
        .value("Compound", StoreType::Compound)
        .value("Array", StoreType::Array)
        .finalize();

    py::class_<Member>(m, "Member")
        .def(py::init<TypeCode::code_t, std::string>())
        .def(py::init([](TypeCode::code_t code, const std::string& name,
                         const std::vector<Member>& children){
            return Member(code, name, children);
        }));

    py::class_<TypeCode>(m, "TypeCode")
        .def(py::init<TypeCode::code_t>())
        .def_readonly("code", &TypeCode::code)
        .def("name", &TypeCode::name)
        .def("is_unsigned", &TypeCode::isunsigned)
        .def("is_array", &TypeCode::isarray);

    py::class_<TypeDef>(m, "TypeDef")
        .def(py::init<TypeCode::code_t>())
        .def(py::init([](TypeCode::code_t code,
                         const std::vector<Member>& children){
            return TypeDef(code, std::string(), children);
        }))
        .def("create", &TypeDef::create)
        .def("__repr__", [](const TypeDef& self){
            std::stringstream ss;
            ss << self;
            return ss.str();
        });

    py::class_<Value>(m, "Value", "Generic data container")

        .def(py::init<const Value&>())
        .def("type", &Value::type)
        .def("storageType", &Value::storageType)
        .def("id", &Value::id)

        .def("cloneEmpty", &Value::cloneEmpty,
                           "Return empty-initialised Value with same TypeDef")

        .def("__iter__", [](const Value& self) {
            return py::make_iterator(self.ichildren().begin(), self.ichildren().end());
        }, py::keep_alive<0, 1>(), "Iterate through outer-most fields in Value")

        .def("__getattr__", static_cast<Value (Value::*)(const std::string&)>(&Value::lookup),
                            "Lookup and return field in Value (no casting)")
        .def("__getitem__", static_cast<Value (Value::*)(const std::string&)>(&Value::lookup),
                            "Lookup and return field in Value (no casting)")

        .def("assign", &Value::assign, "Assign new Value to Value (no casting)")
        .def("assign", [](const Value& self, py::dict values_dict) {
            for (auto item : values_dict) {
                const std::string key = item.first.cast<std::string>();
                const py::object& py_value = py::reinterpret_borrow<py::object>(item.second);
                //try {
                    py::cast(self).attr("__setattr__")(key, py_value);
                //}
                /*catch (py::error_already_set& e) {
                    std::stringstream ss;
                    if (e.matches(PyExc_KeyError)) {
                        ss << "No such field '" << key << "'";
                        py::raise_from(e, PyExc_KeyError, ss.str().c_str());
                    }
                    else if (e.matches(PyExc_TypeError)) {
                        auto pvxs_typename = self.lookup(key).type();
                        auto py_typename = py::str(py_value.attr("__class__").attr("__name__"));
                        ss << "Unable to assign " << pvxs_typename << " field '" << key << "' ";
                        ss << "with " << py_typename << " '" << py_value << "'";
                        py::raise_from(e, PyExc_TypeError, ss.str().c_str());
                    }
                    throw py::error_already_set();
                }*/
            }
        }, "Iterate through python dictionary and cast values to Value fields")

        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const int64_t&)>(&Value::update<const int64_t&, std::string&>),
                            "Lookup field in Value and cast python int to Value")
        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const double&)>(&Value::update<const double&, std::string&>),
                            "Lookup field in Value and cast python float to Value")
        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const std::string&)>(&Value::update<const std::string&, std::string&>),
                            "Lookup field in Value and cast python string to Value")
        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const shared_array<const void>&)>(&Value::update<const shared_array<const void>&, std::string&>),
                            "Lookup field in Value and cast python list or array to Value")
        .def("__setattr__", [](const Value& self, std::string& name, py::dict values_dict) {
            py::cast(self.lookup(name)).attr("assign")(values_dict);
        }, "Lookup field in Value and cast python dictionary to Value")

        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const int64_t&)>(&Value::update<const int64_t&, std::string&>),
                            "Lookup field in Value and cast python int to Value")
        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const double&)>(&Value::update<const double&, std::string&>),
                            "Lookup field in Value and cast python float to Value")
        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const std::string&)>(&Value::update<const std::string&, std::string&>),
                            "Lookup field in Value and cast python string to Value")
        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const shared_array<const void>&)>(&Value::update<const shared_array<const void>&, std::string&>),
                            "Lookup field in Value and cast python list or array to Value")
        .def("__setitem__", [](const Value& self, std::string& name, py::dict values_dict) {
            py::cast(self.lookup(name)).attr("assign")(values_dict);
        }, "Lookup field in Value and cast python dictionary to Value")

        .def("get", [](const Value& self, const std::string& name, py::object def_value) {
            try {
                return py::cast(self.lookup(name));
            }
            catch (const std::exception& e) {
                return def_value;
            }
        }, py::arg("name"), py::arg("def_value") = py::none(),
           "Lookup field from Value and cast to python value if found, otherwise return python None")

        .def("as_bool", static_cast<bool (Value::*)(void) const>(&Value::as<bool>),
                        "Returns a python bool representation of Value")
        .def("__bool__", static_cast<bool (Value::*)(void) const>(&Value::as<bool>),
                         "Cast Value to python bool")

        .def("as_int", static_cast<int64_t (Value::*)(void) const>(&Value::as<int64_t>),
                       "Returns a python int representation of Value")
        .def("__int__", static_cast<int64_t (Value::*)(void) const>(&Value::as<int64_t>),
                        "Cast Value to python int")

        .def("as_list", [](const Value& self){
            auto sa = self.as<shared_array<const void>>();
            if (sa.original_type() == ArrayType::String)
                return py::cast(sa);
            else if (sa.original_type() == ArrayType::Null)
                return py::list().cast<py::object>();
            else
                return py::cast(sa).attr("tolist")();
        }, "Returns a python list representation of Value")

        .def("as_dict", [](const Value& self){
            py::dict py_dict;
            for (auto item : self.ichildren()) {
                auto key = py::str(self.nameOf(item));
                if (item.nmembers() > 0) {
                    py_dict[key] = py::cast(item).attr("as_dict")();
                }
                else {
                    switch(item.storageType()) {
                        case StoreType::Bool:
                            py_dict[key] = py::cast(item.as<bool>());
                            break;
                        case StoreType::UInteger:
                        case StoreType::Integer:
                            py_dict[key] = py::cast(item.as<int64_t>());
                            break;
                        case StoreType::Real:
                            py_dict[key] = py::cast(item.as<double>());
                            break;
                        case StoreType::String:
                            py_dict[key] = py::cast(item.as<std::string>());
                            break;
                        case StoreType::Array:
                            py_dict[key] = py::cast(item).attr("as_list")();
                            break;
                        default:
                            py_dict[key] = py::cast(item);
                    }
                }
            }
            return py_dict;
        }, "Returns a python dictionary representation of Value")

        .def("as_array", static_cast<shared_array<const void> (Value::*)(void) const>(&Value::as<shared_array<const void>>),
                         "Returns a python array.array() representation of Value")

        // convenient to call these instead of .as_array().tolist()
        .def("as_int_list", [](const Value& self){
            py::object array_array = py::module_::import("array").attr("array");
            return array_array("q", self.as<shared_array<const void>>()).attr("tolist")();
        }, "Returns a python list[int] representation of Value")
        .def("as_float_list", [](const Value& self){
            py::object array_array = py::module_::import("array").attr("array");
            return array_array("d", self.as<shared_array<const void>>()).attr("tolist")();
        }, "Returns a python list[fload] representation of Value")
        .def("as_string_list", [](const Value& self){
            auto sa = self.as<shared_array<const void>>();
            if (sa.original_type() == ArrayType::String) {
                return py::cast(sa);
            }
            else {
                py::object array_array = py::module_::import("array").attr("array");
                return array_array("u", self.as<shared_array<const void>>()).attr("tolist")();
            }
        }, "Returns a python list[str] representation of Value")

        .def("as_float", static_cast<double (Value::*)(void) const>(&Value::as<double>),
                         "Returns a python float representation of Value")
        .def("__float__", static_cast<double (Value::*)(void) const>(&Value::as<double>),
                          "Cast Value to python float")

        .def("as_string", static_cast<std::string (Value::*)(void) const>(&Value::as<std::string>),
                          "Returns a python string representation of Value")

        // string representation of values, very basic implementation, could be better
        .def("__str__", [](const Value& self){
            std::stringstream ss;
            try {
                ss << self.as<std::string>();
            } catch (...) {
                ss << self;
            }
            return ss.str();
        }, "Cast Value to python string")
        .def("__repr__", [](const Value& self){
            std::stringstream ss;
            ss << self;
            return ss.str();
        }, "Returns a string representation of Value");
}
