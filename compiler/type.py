import enum
import struct
from typing import Union


class Type(enum.IntEnum):
    i8 = 0
    u8 = 1
    i16 = 2
    u16 = 3
    i32 = 4
    u32 = 5
    i64 = 6
    u64 = 7
    f32 = 8
    f64 = 9
    bool = 10
    string = 11
    bytes = 12

    def serialize(self, value: Union[str, bytes, bool, float, int]):
        if self in TYPE_SERIALIZATIONS:
            assert type(self.value) in (int, float)
            return struct.pack(TYPE_SERIALIZATIONS[self], self.value)
        else:
            if self == Type.bool:
                assert type(self.value) == bool
                return struct.pack("B", 1 if self.value else 0)
            elif self == Type.string:
                assert type(self.value) == str
                return len(value).to_bytes(4, byteorder="big", signed=False) + value.encode()
            else:
                return value


TYPE_SERIALIZATIONS = {
    Type.i8: "b",
    Type.u8: "B",
    Type.i16: "h",
    Type.u16: "H",
    Type.i32: "i",
    Type.u32: "I",
    Type.i64: "q",
    Type.u64: "Q",
    Type.f32: "f",
    Type.f64: "d",
}


UNSIGNED_TYPES = {
    Type.u8,
    Type.u16,
    Type.u32,
    Type.u64,
}

SIGNED_TYPES = {
    Type.i8,
    Type.i16,
    Type.i32,
    Type.i64,
}

FLOATING_TYPES = {
    Type.f32,
    Type.f64,
}


INTEGRAL_TYPES = {
    *SIGNED_TYPES,
    *UNSIGNED_TYPES,
    *FLOATING_TYPES
}

IMMEDIATE_TYPES = {
    *INTEGRAL_TYPES,
    Type.bool
}
