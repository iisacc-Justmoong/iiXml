"""Python access to iiXml's native, immutable numeric matrix."""

import ctypes as _ct
import operator as _operator
import os as _os
from pathlib import Path as _Path


class MatrixError(ValueError):
    """The native matrix operation could not be completed."""


class _Number(_ct.Structure):
    _fields_ = [("kind", _ct.c_int), ("integer", _ct.c_int64), ("real", _ct.c_double)]


def _load_library():
    override = _os.environ.get("IIXML_MATRIX_LIBRARY")
    if override:
        return _ct.CDLL(override)
    module_dir = _Path(__file__).resolve().parent
    libdir = module_dir.parents[1]
    if _os.name == "nt":
        names = [module_dir / "iiXmlMatrixC.dll", libdir.parent / "bin" / "iiXmlMatrixC.dll",
                 libdir / "iiXmlMatrixC.dll"]
    else:
        names = [module_dir / "libiiXmlMatrixC.dylib", module_dir / "libiiXmlMatrixC.so",
                 libdir / "libiiXmlMatrixC.dylib", libdir / "libiiXmlMatrixC.so"]
    for path in names:
        if path.is_file():
            return _ct.CDLL(str(path))
    raise ImportError("iiXmlMatrixC was not found; set IIXML_MATRIX_LIBRARY to the built library")


_lib = _load_library()
_handle = _ct.c_void_p
_size = _ct.c_size_t
_size_ptr = _ct.POINTER(_size)
_axis = _ct.c_ssize_t
_axis_ptr = _ct.POINTER(_axis)
_number_ptr = _ct.POINTER(_Number)
_handle_ptr = _ct.POINTER(_handle)


def _signature(name, arguments, result=_ct.c_int):
    function = getattr(_lib, name)
    function.argtypes = arguments
    function.restype = result
    return function


_error = _signature("iixml_matrix_error", [], _ct.c_char_p)
_create = _signature("iixml_matrix_create", [_size_ptr, _size, _number_ptr, _size, _handle_ptr])
_destroy = _signature("iixml_matrix_destroy", [_handle], None)
_rank = _signature("iixml_matrix_rank", [_handle], _size)
_size_of = _signature("iixml_matrix_size", [_handle], _size)
_dimension = _signature("iixml_matrix_dimension", [_handle, _size, _size_ptr])
_value = _signature("iixml_matrix_value", [_handle, _size, _number_ptr])
_at = _signature("iixml_matrix_at", [_handle, _size_ptr, _size, _number_ptr])
_reshape = _signature("iixml_matrix_reshape", [_handle, _size_ptr, _size, _handle_ptr])
_binary = _signature("iixml_matrix_binary", [_handle, _handle, _ct.c_int, _handle_ptr])
_reduce = _signature("iixml_matrix_reduce", [_handle, _ct.c_int, _number_ptr])
_reduce_axes = _signature("iixml_matrix_reduce_axes", [_handle, _ct.c_int, _axis_ptr, _size, _ct.c_int, _handle_ptr])
_tensordot = _signature("iixml_matrix_tensordot", [_handle, _handle, _axis_ptr, _size, _axis_ptr, _size, _handle_ptr])
_from_xml = _signature("iixml_matrix_from_xml", [_ct.c_char_p, _size, _handle_ptr])
_to_xml = _signature("iixml_matrix_to_xml", [_handle, _handle_ptr, _size_ptr])
_free_string = _signature("iixml_matrix_free_string", [_handle], None)


def _check(status):
    if status:
        raise MatrixError((_error() or b"Matrix operation failed").decode("utf-8", "replace"))


def _dimensions(shape):
    dimensions = []
    for value in shape:
        if isinstance(value, bool):
            raise TypeError("Boolean dimensions are not supported")
        value = _operator.index(value)
        if value < 0 or value > _ct.c_size_t(-1).value:
            raise ValueError("Dimension is outside the size_t range")
        dimensions.append(value)
    return (_size * len(dimensions))(*dimensions)


def _as_number(value):
    if isinstance(value, bool):
        raise TypeError("Boolean values are not numeric matrix values")
    if isinstance(value, int):
        if not -(1 << 63) <= value < (1 << 63):
            raise OverflowError("Integer is outside the signed 64-bit range")
        return _Number(0, value, 0.0)
    if isinstance(value, float):
        return _Number(1, 0, value)
    raise TypeError("Matrix values must be int or float")


def _axes(values):
    if isinstance(values, bool):
        raise TypeError("Boolean axes are not supported")
    try:
        values = (_operator.index(values),)
    except TypeError:
        pass
    bits = _ct.sizeof(_axis) * 8
    result = []
    for value in values:
        if isinstance(value, bool):
            raise TypeError("Boolean axes are not supported")
        value = _operator.index(value)
        if value < -(1 << (bits - 1)) or value >= (1 << (bits - 1)):
            raise OverflowError("Axis is outside the ptrdiff_t range")
        result.append(value)
    return (_axis * len(result))(*result)


def _python_number(value):
    return value.integer if value.kind == 0 else value.real


class Matrix:
    """An immutable row-major matrix containing int64 and finite double values."""

    __slots__ = ("_handle",)

    def __init__(self, shape, values):
        if getattr(self, "_handle", None):
            raise RuntimeError("An existing Matrix cannot be reinitialized")
        dimensions = _dimensions(shape)
        numbers = [_as_number(value) for value in values]
        data = (_Number * len(numbers))(*numbers)
        handle = _handle()
        _check(_create(dimensions, len(dimensions), data, len(data), _ct.byref(handle)))
        self._handle = handle

    @classmethod
    def _from_handle(cls, handle):
        result = cls.__new__(cls)
        result._handle = handle
        return result

    def __del__(self):
        handle = getattr(self, "_handle", None)
        if handle:
            _destroy(handle)
            self._handle = None

    def __copy__(self):
        return self

    def __deepcopy__(self, memo):
        return self

    def __reduce__(self):
        return type(self), (self.shape, self.values)

    @property
    def shape(self):
        result = []
        for index in range(_rank(self._handle)):
            dimension = _size()
            _check(_dimension(self._handle, index, _ct.byref(dimension)))
            result.append(dimension.value)
        return tuple(result)

    @property
    def values(self):
        result = []
        for index in range(_size_of(self._handle)):
            number = _Number()
            _check(_value(self._handle, index, _ct.byref(number)))
            result.append(_python_number(number))
        return result

    def at(self, index):
        dimensions = _dimensions(index)
        number = _Number()
        _check(_at(self._handle, dimensions, len(dimensions), _ct.byref(number)))
        return _python_number(number)

    def reshape(self, shape):
        dimensions = _dimensions(shape)
        output = _handle()
        _check(_reshape(self._handle, dimensions, len(dimensions), _ct.byref(output)))
        return self._from_handle(output)

    @classmethod
    def from_xml(cls, xml):
        if not isinstance(xml, str):
            raise TypeError("XML input must be a string")
        encoded = xml.encode("utf-8")
        output = _handle()
        _check(_from_xml(encoded, len(encoded), _ct.byref(output)))
        return cls._from_handle(output)

    def to_xml(self):
        output = _handle()
        length = _size()
        _check(_to_xml(self._handle, _ct.byref(output), _ct.byref(length)))
        try:
            return _ct.string_at(output, length.value).decode("utf-8")
        finally:
            _free_string(output)

    def _operation(self, other, operation, reverse=False):
        if not isinstance(other, Matrix):
            if isinstance(other, bool) or not isinstance(other, (int, float)):
                return NotImplemented
            other = Matrix((), [other])
        left, right = (other, self) if reverse else (self, other)
        output = _handle()
        _check(_binary(left._handle, right._handle, operation, _ct.byref(output)))
        return self._from_handle(output)

    def __add__(self, other): return self._operation(other, 0)
    def __radd__(self, other): return self._operation(other, 0, True)
    def __sub__(self, other): return self._operation(other, 1)
    def __rsub__(self, other): return self._operation(other, 1, True)
    def __mul__(self, other): return self._operation(other, 2)
    def __rmul__(self, other): return self._operation(other, 2, True)
    def __truediv__(self, other): return self._operation(other, 3)
    def __rtruediv__(self, other): return self._operation(other, 3, True)

    def matmul(self, other):
        if not isinstance(other, Matrix):
            raise TypeError("Matrix multiplication requires a Matrix")
        return self._operation(other, 4)

    def __matmul__(self, other):
        return self.matmul(other) if isinstance(other, Matrix) else NotImplemented

    def tensordot(self, other, left_axes, right_axes):
        if not isinstance(other, Matrix):
            raise TypeError("Tensor contraction requires a Matrix")
        left = _axes(left_axes)
        right = _axes(right_axes)
        output = _handle()
        _check(_tensordot(self._handle, other._handle, left, len(left), right, len(right), _ct.byref(output)))
        return self._from_handle(output)

    def _reduction(self, operation, axis, keepdims):
        if not isinstance(keepdims, bool):
            raise TypeError("keepdims must be a boolean")
        if axis is None and not keepdims:
            output = _Number()
            _check(_reduce(self._handle, operation, _ct.byref(output)))
            return _python_number(output)
        selected = _axes(range(_rank(self._handle)) if axis is None else axis)
        output = _handle()
        _check(_reduce_axes(self._handle, operation, selected, len(selected), int(keepdims), _ct.byref(output)))
        return self._from_handle(output)

    def sum(self, axis=None, keepdims=False):
        return self._reduction(0, axis, keepdims)

    def mean(self, axis=None, keepdims=False):
        return self._reduction(1, axis, keepdims)
