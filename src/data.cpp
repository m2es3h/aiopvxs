#include <iostream>

#include <pybind11/pybind11.h>
#include <pybind11/native_enum.h>
#include <pybind11/stl.h>

#include <pvxs/data.h>

#include "pvxs_types.h"

namespace py = pybind11;


void create_submodule_data(py::module_& m) {
    using namespace pvxs;

    py::native_enum<TypeCode::code_t>(m, "TypeCodeEnum", "enum.IntEnum")
        .value("Bool", TypeCode::code_t::Bool)
        .value("UInt8", TypeCode::code_t::UInt8)
        .value("UInt16", TypeCode::code_t::UInt16)
        .value("UInt32", TypeCode::code_t::UInt32)
        .value("UInt64", TypeCode::code_t::UInt64)
        .value("Int8", TypeCode::code_t::Int8)
        .value("Int8A", TypeCode::code_t::Int8A)
        .value("Int16", TypeCode::code_t::Int16)
        .value("Int16A", TypeCode::code_t::Int16A)
        .value("Int32", TypeCode::code_t::Int32)
        .value("Int32A", TypeCode::code_t::Int32A)
        .value("Int64", TypeCode::code_t::Int64)
        .value("Int64A", TypeCode::code_t::Int64A)
        .value("Float32", TypeCode::code_t::Float32)
        .value("Float64", TypeCode::code_t::Float64)
        .value("String", TypeCode::code_t::String)
        .value("Struct", TypeCode::code_t::Struct)
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
        .def(py::init([](TypeCode::code_t code, const std::string& name, const std::vector<Member>& children){
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
        .def(py::init([](TypeCode::code_t code, const std::vector<Member>& children){
            return TypeDef(code, std::string(), children);
        }))
        .def("create", &TypeDef::create)
        .def("__repr__", [](const TypeDef& self){
            std::stringstream ss;
            ss << self;
            return ss.str();
        });

    py::class_<Value>(m, "Value")
        //.def(py::init<Value>())
        .def("type", &Value::type)
        .def("storageType", &Value::storageType)
        .def("id", &Value::id)

        .def("__iter__", [](const Value& self) {
            /*std::cout << "STORE TYPE " << self.storageType() << std::endl;
            if (self.storageType() == StoreType::Array) {
                auto arr = self.as<shared_array<const int64_t>>();
                py::list py_list = py::cast(arr);
                return py::make_iterator(py_list.begin(), py_list.end());
            }
            else {
                py::list py_list;
                for(auto val : self.ichildren()) {
                    std::cout << "making py_list of Value" << std::endl;
                    py_list.append(val);
                }
                return py::make_iterator(py_list.begin(), py_list.end());
            }*/
           return py::make_iterator(self.ichildren().begin(), self.ichildren().end());
        }, py::keep_alive<0, 1>())

        .def("__getattr__", [](const Value& self, const std::string& name) {
            return self.lookup(name);
        })
        .def("__getitem__", [](const Value& self, const std::string& name) {
            return self.lookup(name);
        })

        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const int64_t&)>(&Value::update<const int64_t&, std::string&>))
        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const double&)>(&Value::update<const double&, std::string&>))
        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const std::string&)>(&Value::update<const std::string&, std::string&>))
        .def("__setattr__", static_cast<Value& (Value::*)(std::string&, const shared_array<const int64_t>&)>(&Value::update<const shared_array<const int64_t>&, std::string&>))

        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const int64_t&)>(&Value::update<const int64_t&, std::string&>))
        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const double&)>(&Value::update<const double&, std::string&>))
        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const std::string&)>(&Value::update<const std::string&, std::string&>))
        .def("__setitem__", static_cast<Value& (Value::*)(std::string&, const shared_array<const int64_t>&)>(&Value::update<const shared_array<const int64_t>&, std::string&>))

        .def("get", [](const Value& self, const std::string& name) {
            return self.lookup(name);
        })

        .def("as_bool", static_cast<bool (Value::*)(void) const>(&Value::as<bool>))
        .def("__bool__",  static_cast<bool (Value::*)(void) const>(&Value::as<bool>))

        //.def("as_int32", static_cast<int32_t (Value::*)(void) const>(&Value::as<int32_t>))
        //.def("from_int32", static_cast<void (Value::*)(const int32_t&)>(&Value::from<int32_t>))
        .def("as_int", static_cast<int64_t (Value::*)(void) const>(&Value::as<int64_t>))
        .def("__int__",  static_cast<int64_t (Value::*)(void) const>(&Value::as<int64_t>))

        .def("as_int_list", static_cast<shared_array<const int64_t> (Value::*)(void) const>(&Value::as<shared_array<const int64_t>>))
        .def("as_string_list", static_cast<shared_array<const std::string> (Value::*)(void) const>(&Value::as<shared_array<const std::string>>))

        //.def("as_float32", static_cast<float (Value::*)(void) const>(&Value::as<float>))
        .def("as_float", static_cast<double (Value::*)(void) const>(&Value::as<double>))
        .def("__float__",  static_cast<double (Value::*)(void) const>(&Value::as<double>))

        .def("as_string", static_cast<std::string (Value::*)(void) const>(&Value::as<std::string>))
        .def("__str__", [](const Value& self){
            std::stringstream ss;
            try {
                ss << self["value"].as<std::string>();
            } catch (...) {
                ss << self.as<std::string>();
            }
            return ss.str();
        })
        .def("__repr__", [](const Value& self){
            std::stringstream ss;
            ss << self;
            return ss.str();
        });
}
