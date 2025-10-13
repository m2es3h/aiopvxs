#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <pvxs/data.h>

namespace py = pybind11;
using namespace pvxs;

namespace pybind11 {
namespace detail {

#define SHARED_ARRAY_TYPE_CASTER(T, py_hint)                                                \
template <>                                                                                 \
struct type_caster<shared_array<const T>> {                                                 \
    PYBIND11_TYPE_CASTER(shared_array<const T>, io_name(py_hint, "aiopvxs.data.Value"));    \
                                                                                            \
    static handle                                                                           \
    cast(const shared_array<const T>& arr, return_value_policy policy, handle parent) {     \
        /* build a python list from shared_array<T> elements */                             \
        py::list py_list;                                                                   \
        for (auto val : arr) {                                                              \
            py_list.append(val);                                                            \
        }                                                                                   \
        return py_list.release();                                                           \
    }                                                                                       \
                                                                                            \
    bool load(handle src, bool convert) {                                                   \
        /* check if py_object is a buffer */                                                \
        if (py::isinstance<py::buffer>(src)) {                                              \
            py::buffer_info info = py::reinterpret_borrow<py::buffer>(src).request();       \
            /* verify buffer is 1D */                                                       \
            if (info.ndim != 1) {                                                           \
                return false;                                                               \
            }                                                                               \
            /* check this load() method matches buffer data type */                         \
            else if (!info.item_type_is_equivalent_to<T>()) {                               \
                return false;                                                               \
            }                                                                               \
            /* create new shared_array<T> and update value */                               \
            auto arr_begin = static_cast<const T*>(info.ptr);                               \
            auto arr_end = static_cast<const T*>(arr_begin + info.shape[0]);                \
            value = shared_array<const T>(arr_begin, arr_end);                              \
            return true;                                                                    \
        }                                                                                   \
        /* check if py_object is a sequence */                                              \
        else if (py::isinstance<py::sequence>(src)) {                                       \
            /* sequences not yet supported */                                               \
            return false;                                                                   \
        }                                                                                   \
        else {                                                                              \
            return false;                                                                   \
        }                                                                                   \
    }                                                                                       \
};

SHARED_ARRAY_TYPE_CASTER(int8_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(int16_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(int32_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(int64_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint8_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint16_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint32_t, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(uint64_t, "collections.abc.Buffer");

SHARED_ARRAY_TYPE_CASTER(float, "collections.abc.Buffer");
SHARED_ARRAY_TYPE_CASTER(double, "collections.abc.Buffer");


template <>
struct type_caster<shared_array<const void>> {
    PYBIND11_TYPE_CASTER(shared_array<const void>, io_name("collections.abc.Buffer", "aiopvxs.data.Value"));

    static handle
    cast(const shared_array<const void>& sa, return_value_policy policy, handle parent) {
        py::list py_list;

        if (sa.original_type() == ArrayType::UInt8) {
            for (auto val : sa.castTo<const uint8_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::UInt16) {
            for (auto val : sa.castTo<const uint16_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::UInt32) {
            for (auto val : sa.castTo<const uint32_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::UInt64) {
            for (auto val : sa.castTo<const uint64_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Int8) {
            for (auto val : sa.castTo<const int8_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Int16) {
            for (auto val : sa.castTo<const int16_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Int32) {
            for (auto val : sa.castTo<const int32_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Int64) {
            for (auto val : sa.castTo<const int64_t>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Float32) {
            for (auto val : sa.castTo<const float>()) {
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Float64) {
            for (auto val : sa.castTo<const double>()) {
                py_list.append(val);
            }
        }

        return py_list.release();
    }

    bool load(handle src, bool convert) {
        /* check if py_object is a buffer */
        if (py::isinstance<py::buffer>(src)) {
            py::buffer_info info = py::reinterpret_borrow<py::buffer>(src).request();
            /* verify buffer is 1D */
            if (info.ndim != 1) {
                return false;
            }
            /* check this load() method matches buffer data type */
            //if (value.original_type() == ArrayType::UInt8) {
            if (info.item_type_is_equivalent_to<uint8_t>()) {
                auto arr_begin = static_cast<const uint8_t*>(info.ptr);                               \
                auto arr_end = static_cast<const uint8_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const uint8_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::UInt16) {
            else if (info.item_type_is_equivalent_to<uint16_t>()) {
                auto arr_begin = static_cast<const uint16_t*>(info.ptr);                               \
                auto arr_end = static_cast<const uint16_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const uint16_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::UInt32) {
            else if (info.item_type_is_equivalent_to<uint32_t>()) {
                auto arr_begin = static_cast<const uint32_t*>(info.ptr);                               \
                auto arr_end = static_cast<const uint32_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const uint32_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::UInt64) {
            else if (info.item_type_is_equivalent_to<uint64_t>()) {
                auto arr_begin = static_cast<const uint64_t*>(info.ptr);                               \
                auto arr_end = static_cast<const uint64_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const uint64_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::Int8) {
            else if (info.item_type_is_equivalent_to<int8_t>()) {
                auto arr_begin = static_cast<const int8_t*>(info.ptr);                               \
                auto arr_end = static_cast<const int8_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const int8_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::Int16) {
            else if (info.item_type_is_equivalent_to<int16_t>()) {
                auto arr_begin = static_cast<const int16_t*>(info.ptr);                               \
                auto arr_end = static_cast<const int16_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const int16_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::Int32) {
            else if (info.item_type_is_equivalent_to<int32_t>()) {
                auto arr_begin = static_cast<const int32_t*>(info.ptr);                               \
                auto arr_end = static_cast<const int32_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const int32_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::Int64) {
            else if (info.item_type_is_equivalent_to<int64_t>()) {
                auto arr_begin = static_cast<const int64_t*>(info.ptr);                               \
                auto arr_end = static_cast<const int64_t*>(arr_begin + info.shape[0]);                \
                value = shared_array<const int64_t>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::Float32) {
            else if (info.item_type_is_equivalent_to<float>()) {
                auto arr_begin = static_cast<const float*>(info.ptr);                               \
                auto arr_end = static_cast<const float*>(arr_begin + info.shape[0]);                \
                value = shared_array<const float>(arr_begin, arr_end).castTo<const void>();                              \
            }
            //else if (value.original_type() == ArrayType::Float64) {
            else if (info.item_type_is_equivalent_to<double>()) {
                auto arr_begin = static_cast<const double*>(info.ptr);                               \
                auto arr_end = static_cast<const double*>(arr_begin + info.shape[0]);                \
                value = shared_array<const double>(arr_begin, arr_end).castTo<const void>();                              \
            }
            else {
                return false;
            }

            return true;
        }
        /* check if py_object is a sequence */
        else if (py::isinstance<py::sequence>(src)) {
            /* sequences not yet supported */
            return false;
        }
        else {
            return false;
        }
    }
};

} // namespace detail
} // namespace pybind11
