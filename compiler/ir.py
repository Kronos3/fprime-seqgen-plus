from abc import ABC, abstractmethod
from typing import List, Optional, Dict, Union, Tuple
import weakref

from compiler.common import Value, Register, Serializable
from compiler import instructions
from compiler.type import Type, IMMEDIATE_TYPES

from lark import tree


class IR(ABC):
    @abstractmethod
    def encode(self) -> bytearray:
        """
        Encode IR into serialized bytes
        :return: encoded ir
        """


class BasicBlock:
    id: int
    instructions: List[instructions.Instruction]
    next: Optional['BasicBlock']

    def __init__(self, id_: int):
        self.instructions = []
        self.id = id_
        self.next = None

    def encode(self) -> bytearray:
        assert self.id is not None, "BasicBlock id not assigned"

        out = bytearray()
        for i in self.instructions:
            out.extend(i.encode())

        return out


class Variable(Value):
    ast: tree.Token
    name: str
    id: Optional[int]

    def __init__(self, ast: tree.Token):
        super().__init__(Type[ast.type])

        self.id = None
        self.name = ast.value

    def as_register(self, builder: 'Builder') -> Register:
        assert self.id is not None
        return builder.append(instructions.LoadVariable(self.id, self.ty))

    def build(self, builder: 'Builder'):
        pass

    def serialize(self) -> bytes:
        assert self.id is not None
        return self.id.to_bytes(4, byteorder="big", signed=False)


class Constant(Value):
    value: Union[str, float, int, bool]
    _const_id: Optional[int]

    def __init__(self, value: Union[str, float, int, bool], ty: Type):
        super().__init__(ty)
        self.value = value
        self._const_id = None

        assert ty != Type.bytes, "How did you do this"

    def as_register(self, builder: 'Builder') -> 'Register':
        if self.ty in IMMEDIATE_TYPES:
            return builder.append(instructions.LoadImmediate(self.value, self.ty))
        elif self._const_id is not None:
            # String values are just indexes in const memory
            return builder.append(instructions.LoadImmediate(self._const_id, self.ty))
        else:
            assert False, self.ty

    def serialize(self) -> bytes:
        return self.ty.serialize(self.value)


class Label(Serializable):
    label_id: int
    block: Optional[BasicBlock]
    index: Optional[int]

    def __init__(self, label_id: int):
        self.label_id = label_id
        self.index = None
        self.block = None

    def hook(self, block: BasicBlock):
        assert self.block is None, self.block
        assert self.index is None, self.index
        self.block = block
        self.index = len(block.instructions)

    def serialize(self) -> bytes:
        assert self.block is not None
        assert self.index is not None
        return self.label_id.to_bytes(4, byteorder="big", signed=False)


class Context:
    """
    A context is a place for variables to live
    It is essentially an indentation level
    Context's can have multiple children which inherit
    variables from this context.

    The root context is the global scope.
    """

    parent: Optional[weakref.ref['Context']]
    children: List['Context']
    variables: Dict[str, Variable]

    def __init__(self, parent: Optional['Context']):
        if parent:
            self.parent: weakref.ref['Context'] = weakref.ref(parent)

            # The parent holds a strong reference to the children
            self.parent().children.append(self)
        else:
            self.parent = None
        self.variables = {}

    def find(self, name: str) -> Optional[Variable]:
        if name in self.variables:
            return self.variables[name]

        if self.parent:
            p: Context = self.parent()
            assert p is not None, "Parent is cleared before child"
            return p.find(name)
        else:
            return None


class Builder:
    """
    IR builder that holds the BasicBlock
    and contexts during compilation
    """

    context: Context
    current: BasicBlock

    labels: List[Label]
    blocks: List[BasicBlock]
    const_data: List[Constant]

    _root_block: BasicBlock
    _root_context: Context

    loop_labels: Optional[Tuple[Label, Label]]

    def __init__(self):
        self.labels = []
        self.blocks = []
        self.const_data = []

        # Top level execution block
        self.current = BasicBlock(0)
        self._root_block = self.current
        self.blocks.append(self.current)

        self.context = Context(None)
        self._root_context = self.context

        self.loop_labels = None

    def push_context(self):
        """
        Enter a new context
        This context is a child of the current context
        It inherits all of its variables so that it may access them
        """
        assert self.context
        self.context = Context(self.context)

    def pop_context(self):
        """
        Exit the current context
        Moves to the parent context
        :return:
        """
        assert self.context
        assert self.context.parent, "Out of contexts"
        self.context = self.context.parent()

    def new_block(self, chain_block: bool = True) -> BasicBlock:
        bb = BasicBlock(len(self.blocks))
        if chain_block:
            self.current.next = bb
        self.blocks.append(bb)
        self.current = bb
        return bb

    def create_label(self) -> Label:
        label = Label(len(self.labels))
        self.labels.append(label)
        return label

    def append(self, instr):
        self.current.instructions.append(instr)
        return instr

    @property
    def root_block(self) -> BasicBlock:
        return self._root_block

    @property
    def root_context(self) -> Context:
        return self._root_context
