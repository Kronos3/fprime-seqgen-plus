import enum
import struct
from abc import ABC
from typing import List, Iterable, Union

from compiler.common import Serializable, Register, Value
from compiler.ir import IR, Label
from compiler.type import Type


class Opcode(enum.IntEnum):
    NOP = 0x0

    # Compare operations
    LT = enum.auto()
    GT = enum.auto()
    LE = enum.auto()
    GE = enum.auto()
    EQ = enum.auto()
    NEQ = enum.auto()

    # Logical binary operations
    AND = enum.auto()
    OR = enum.auto()

    # Logical Unary operations
    NOT = enum.auto()

    # Arithmetic binary operations
    DIV = enum.auto()
    MUL = enum.auto()
    ADD = enum.auto()
    SUB = enum.auto()

    # Bitwise operators TODO
    # BIT_AND = enum.auto()
    # BIT_OR = enum.auto()
    # BIT_XOR = enum.auto()
    # SHIFT_RIGHT = enum.auto()
    # SHIFT_LEFT = enum.auto()
    # BIT_NOT = enum.auto()

    # Execute a command using a variable number of
    # registers as operands.
    COMMAND = enum.auto()

    # Grab the latest return value and place it into a register
    COMMAND_RETURN = enum.auto()

    LOAD_IMMEDIATE = enum.auto()
    LOAD_VARIABLE = enum.auto()

    BRANCH_TRUE = enum.auto()
    BRANCH_FALSE = enum.auto()
    JUMP = enum.auto()
    RETURN = enum.auto()


LOGICAL_BINARY = {
    Opcode.LT,
    Opcode.GT,
    Opcode.LE,
    Opcode.GE,
    Opcode.EQ,
    Opcode.NEQ,
    Opcode.AND,
    Opcode.OR
}

ARITHMETIC_BINARY = {
    Opcode.DIV,
    Opcode.MUL,
    Opcode.ADD,
    Opcode.SUB,
}


class Instruction(IR, Serializable, ABC):
    opcode: Opcode

    def __init__(self, opcode: Opcode):
        self.opcode = opcode

    def encode(self) -> bytes:
        return self.opcode.to_bytes(2, byteorder="big", signed=False) + self.serialize()

    @staticmethod
    def encode_types(ty_list: Iterable[Type]) -> bytes:
        """
        Encode a set of types to runtime annotate instructions
        :param ty_list: list of types to encode
        :return: encoded bytes
        """

        out = b""
        curr = 0x0
        shifted = False
        for ty in ty_list:
            curr |= ty
            if not shifted:
                curr <<= 4
                shifted = True
            else:
                out += curr.to_bytes(1, byteorder="big", signed=False)
                shifted = False
                curr = 0

        return out


class BinaryOp(Instruction, Register):
    lhs: Register
    rhs: Register

    def __init__(self,
                 opcode: Opcode,
                 lhs: Register,
                 rhs: Register):
        assert self.lhs.ty == self.rhs.ty

        Instruction.__init__(self, opcode)
        Register.__init__(self, self.lhs.ty)
        self.lhs = lhs
        self.rhs = rhs

        lhs.used_by.append(self)
        rhs.used_by.append(self)

    def serialize(self) -> bytes:
        """
        Binary operations are serialized as follows
        :return:
        """

        assert self.register is not None
        assert self.lhs.register is not None
        assert self.rhs.register is not None

        return struct.pack(
            "BBBB",
            self.lhs.ty,        # Type modifier
            self.register,      # Output operand
            self.lhs.register,  # First operand
            self.rhs.register   # Second operand
        )


class CommandReturn(Instruction, Register):
    def __init__(self):
        Instruction.__init__(self, Opcode.COMMAND_RETURN)
        Register.__init__(self, Type.bytes)


class Command(Instruction, Value):
    cmd_opcode: int
    args: List[Register]

    def __init__(self, cmd_opcode, args: List[Register]):
        super().__init__(Opcode.COMMAND)
        self.cmd_opcode = cmd_opcode
        self.args = args

        for a in self.args:
            a.used_by.append(self)

    def as_register(self, builder) -> 'Register':
        return builder.append(CommandReturn())

    def serialize(self) -> bytes:
        # By the time the registers are passed to this instruction,
        # it should already be the correct type
        type_encoding = self.encode_types([x.ty for x in self.args])
        register_encoding = bytearray()

        for i in self.args:
            register_encoding.extend(i.serialize())

        return len(self.args).to_bytes(1, byteorder="big", signed=False) + type_encoding + register_encoding


class LoadImmediate(Instruction, Register):
    value: Union[float, int, bool]

    def __init__(self, value: Union[float, int, bool], ty: Type):
        Instruction.__init__(self, Opcode.LOAD_IMMEDIATE)
        Register.__init__(self, ty)
        self.value = value

    def serialize(self) -> bytes:
        assert self.register is not None

        type_encoding = self.encode_types((self.ty,))
        value_encoding = self.ty.serialize(self.value)
        return Register.serialize(self) + type_encoding + value_encoding


class LoadVariable(Instruction, Register):
    variable_id: int

    def __init__(self, variable_id: int, ty: Type):
        Instruction.__init__(self, Opcode.LOAD_VARIABLE)
        Register.__init__(self, ty)

        self.variable_id = variable_id

    def serialize(self) -> bytes:
        return Register.serialize(self) + self.variable_id.to_bytes(4, byteorder="big", signed=False)


class Branch(Instruction):
    compare: Register
    label: Label

    def __init__(self, compare: Register, label: Label, branch_if: bool):
        Instruction.__init__(self, Opcode.BRANCH_TRUE if branch_if else Opcode.BRANCH_FALSE)
        self.compare = compare
        self.label = label

    def serialize(self) -> bytes:
        return self.compare.serialize() + self.label.serialize()


class Jump(Instruction):
    label: Label

    def __init__(self, label: Label):
        Instruction.__init__(self, Opcode.JUMP)
        self.label = label

    def serialize(self) -> bytes:
        return self.label.serialize()


class Return(Instruction):
    value: Register

    def __init__(self, value: Register):
        Instruction.__init__(self, Opcode.RETURN)

        self.value = value

    def serialize(self) -> bytes:

