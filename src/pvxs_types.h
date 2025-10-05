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
    cast(const shared_array<const T>& sa, return_value_policy policy, handle parent) {      \
        py::list py_list;                                                                   \
        for(auto val : sa) {                                                                \
            std::cout << "CAST READ " << val << std::endl;                                  \
            py_list.append(val);                                                            \
        }                                                                                   \
        return py_list.release();                                                           \
    }                                                                                       \
                                                                                            \
    bool load(handle src, bool convert) {                                                   \
        /* check if py_object is a sequence */                                              \
        if (!py::isinstance<py::sequence>(src)) {                                           \
            return false;                                                                   \
        }                                                                                   \
        /* check that each sequence item is an integer */                                   \
        py::sequence seq = py::reinterpret_borrow<py::sequence>(src);                       \
        for (auto item : seq) {                                                             \
            if (!py::isinstance<py::int_>(item)) {                                          \
                return false;                                                               \
            }                                                                               \
            std::cout << "LOAD " << item << std::endl;                                      \
        }                                                                                   \
        /* convert */                                                                       \
        std::vector<T> seq_as_vector = seq.cast<std::vector<T>>();                          \
        shared_array<const T> new_value(seq_as_vector.begin(), seq_as_vector.end());        \
        value = new_value;                                                                  \
        return true;                                                                        \
    }                                                                                       \
};

SHARED_ARRAY_TYPE_CASTER(int8_t, "Sequence[typing.SupportsInt]");
SHARED_ARRAY_TYPE_CASTER(int16_t, "Sequence[typing.SupportsInt]");
SHARED_ARRAY_TYPE_CASTER(int32_t, "Sequence[typing.SupportsInt]");
SHARED_ARRAY_TYPE_CASTER(int64_t, "Sequence[typing.SupportsInt]");


template <>
struct type_caster<shared_array<const void>> {
    PYBIND11_TYPE_CASTER(shared_array<const void>, io_name("Sequence[typing.Any]", "aiopvxs.data.Value"));

    static handle
    cast(const shared_array<const void>& sa, return_value_policy policy, handle parent) {
        py::list py_list;

        if (sa.original_type() == ArrayType::Int64) {
            shared_array<const int64_t> val_array(sa.castTo<const int64_t>());
            for (auto val : val_array) {
                std::cout << "CAST READ 64" << val << std::endl;
                py_list.append(val);
            }
        }
        else if (sa.original_type() == ArrayType::Int32) {
            shared_array<const int32_t> val_array(sa.castTo<const int32_t>());
            for (auto val : val_array) {
                std::cout << "CAST READ 32" << val << std::endl;
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
