#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/client.h>

namespace py = pybind11;


void create_submodule_data(py::module_& m) {
    using namespace pvxs;

    py::enum_<TypeCode::code_t>(m, "TypeCodeEnum")
        .value("Bool", TypeCode::Bool)
        .value("UInt8", TypeCode::UInt8)
        .value("UInt16", TypeCode::UInt16)
        .value("UInt32", TypeCode::UInt32)
        .value("UInt64", TypeCode::UInt64)
        .value("Int8", TypeCode::Int8)
        .value("Int16", TypeCode::Int16)
        .value("Int32", TypeCode::Int32)
        .value("Int64", TypeCode::Int64)
        .value("Float32", TypeCode::Float32)
        .value("Float64", TypeCode::Float64)
        .value("String", TypeCode::String)
        .value("Struct", TypeCode::Struct)
        .export_values();

    py::class_<Member>(m, "Member")
        .def(py::init<TypeCode::code_t, std::string>())
        .def(py::init([](TypeCode::code_t code, const std::string& name, const std::vector<Member>& children){
            return Member(code, name, children);
        }));

    py::class_<TypeCode>(m, "TypeCode")
        .def_readonly("code", &TypeCode::code)
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
        .def("id", &Value::id)
        .def("__getattr__", [](const Value& self, const std::string& name) {
            return self.lookup(name);
        })
        .def("__getitem__", [](const Value& self, const std::string& name) {
            return self.lookup(name);
        })
        .def("__setattr__", [](Value& self, const std::string& name, py::object new_value) {
            if (py::isinstance<py::int_>(new_value)) {
                self[name] = py::cast<int64_t>(new_value);
            }
            else if (py::isinstance<py::float_>(new_value)) {
                self[name] = py::cast<double>(new_value);
            }
            else if (py::isinstance<py::str>(new_value)) {
                self[name] = py::cast<std::string>(new_value);
            }
        })
        .def("__setitem__", [](Value& self, const std::string& name, py::object new_value) {
            if (py::isinstance<py::int_>(new_value)) {
                self[name] = py::cast<int64_t>(new_value);
            }
            else if (py::isinstance<py::float_>(new_value)) {
                self[name] = py::cast<double>(new_value);
            }
            else if (py::isinstance<py::str>(new_value)) {
                self[name] = py::cast<std::string>(new_value);
            }
        })
        .def("get", [](const Value& self, const std::string& name) {
            return self.lookup(name);
        })

        .def("as_int32", static_cast<int32_t (Value::*)(void) const>(&Value::as<int32_t>))
        .def("as_int64", static_cast<int64_t (Value::*)(void) const>(&Value::as<int64_t>))
        .def("__int__",  static_cast<int64_t (Value::*)(void) const>(&Value::as<int64_t>))

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
