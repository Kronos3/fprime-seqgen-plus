from abc import ABC, abstractmethod
from typing import Optional, List, Union

from lark import tree

from compiler.type import Type


class Serializable(ABC):
    @abstractmethod
    def serialize(self) -> bytes:
        """
        Serialize object into a byte stream
        :return: byte stream
        """


class Value(Serializable, ABC):
    """
    Values are results of expressions
    They have a storage type
    """

    ty: Type
    used_by: List['Value']

    def __init__(self, ty: Type):
        self.ty = ty
        self.used_by = []

    @abstractmethod
    def as_register(self, builder) -> 'Register':
        """
        Get a register storing this value
        :return: Register to store this value
        """


class Register(Value):
    register: Optional[int]

    def __init__(self, ty: Type):
        super().__init__(ty)
        self.register = None

    def as_register(self, builder) -> 'Register':
        return self

    def serialize(self) -> bytes:
        assert self.register is not None
        return self.register.to_bytes(1, byteorder="big")


class AstException(Exception):
    message: str
    position: Union[tree.Token, tree.Tree]

    def __init__(self, message: str, position: Union[tree.Token, tree.Tree]):
        Exception.__init__(self)

        self.message = message
        self.position = position
