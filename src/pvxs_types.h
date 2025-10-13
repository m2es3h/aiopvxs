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
    PYBIND11_TYPE_CASTER(shared_array<const void>, io_name("Sequence[typing.Any]", "aiopvxs.data.Value"));

    static handle
    cast(const shared_array<const void>& sa, return_value_policy policy, handle parent) {
        py::list py_list;
        std::cout <<"cast ORIGINAL TYPE "<< sa.original_type() << std::endl;
        if (sa.original_type() == ArrayType::Int64) {
            shared_array<const int64_t> val_array(sa.castTo<const int64_t>());
            for (auto val : val_array) {
                std::cout << "CAST READ 64 " << val << std::endl;
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Int32) {
            shared_array<const int32_t> val_array(sa.castTo<const int32_t>());
            for (auto val : val_array) {
                std::cout << "CAST READ 32 " << val << std::endl;
                py_list.append(val);
            }
        }

        return py_list.release();
    }

    bool load(handle src, bool convert) {
        // check if py_object is a sequence
        if (!py::isinstance<py::sequence>(src)) {
            return false;
        }
        // check that each sequence item is an integer
        py::sequence seq = py::reinterpret_borrow<py::sequence>(src);
        for (auto item : seq) {
            if (!py::isinstance<py::int_>(item)) {
                return false;
            }
            std::cout << "LOAD " << item << std::endl;
        }

        // convert
        std::cout <<"load ORIGINAL TYPE "<< value.original_type() << std::endl;
        if (value.original_type() == ArrayType::Int64) {
            std::vector<int64_t> seq_as_vector = seq.cast<std::vector<int64_t>>();
            shared_array<const int64_t> val_array(seq_as_vector.begin(), seq_as_vector.end());
            value = shared_array_static_cast<const void, const int64_t>(val_array);
        }
        else if (value.original_type() == ArrayType::Int32) {
            std::vector<int32_t> seq_as_vector = seq.cast<std::vector<int32_t>>();
            shared_array<const int32_t> val_array(seq_as_vector.begin(), seq_as_vector.end());
            value = shared_array_static_cast<const void, const int32_t>(val_array);
        }

        return true;
    }
};


} // namespace detail
} // namespace pybind11
