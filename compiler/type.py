import enum
from typing import Dict

from llvmlite.ir import types


# noinspection PyArgumentList
class Type(enum.IntEnum):
    void = enum.auto()
    i8 = enum.auto()
    u8 = enum.auto()
    i16 = enum.auto()
    u16 = enum.auto()
    i32 = enum.auto()
    u32 = enum.auto()
    i64 = enum.auto()
    u64 = enum.auto()
    f32 = enum.auto()
    f64 = enum.auto()
    boolean = enum.auto()
    string = enum.auto()

    __LLVM_CONVERSIONS__: Dict[int, 'Type'] = {
        i8: types.IntType(8),
        u8: types.IntType(8),
        i16: types.IntType(16),
        u16: types.IntType(16),
        i32: types.IntType(32),
        u32: types.IntType(32),
        i64: types.IntType(64),
        u64: types.IntType(64),
    }

    __UNSIGNED_TYPES__ = {
        u8, u16, u32, u64,
    }

    __SIGNED_TYPES__ = {
        i8, i16, i32, i64,
    }

    __FLOATING_TYPES__ = {
        f32, f64,
    }

    __INTEGRAL_TYPES__ = {
        *__SIGNED_TYPES__,
        *__UNSIGNED_TYPES__,
        *__FLOATING_TYPES__
    }

    __IMMEDIATE_TYPES__ = {
        *__INTEGRAL_TYPES__, boolean
    }

    @property
    def is_unsigned(self) -> bool:
        return self in self.__UNSIGNED_TYPES__

    @property
    def is_signed(self) -> bool:
        return self in self.__SIGNED_TYPES__

    @property
    def is_floating(self) -> bool:
        return self in self.__FLOATING_TYPES__

    @property
    def is_integer(self) -> bool:
        return self.is_unsigned or self.is_signed

    @property
    def is_integral(self) -> bool:
        return self in self.__INTEGRAL_TYPES__

    def lower(self) -> types.Type:
        """
        Get the LLVM type from the Ast type
        Note: There is no distinction between signed and unsigned
              types once the Ast type is lowered. This is the same
              as LLVM itself.
        :return: LLVM Type instance
        """
        return self.__LLVM_CONVERSIONS__[self]
