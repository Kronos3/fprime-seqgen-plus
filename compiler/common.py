import weakref
from abc import ABC
from typing import Union, Optional, List, Dict, Tuple

from lark import tree, Token
from llvmlite import ir

from compiler.type import Type


class AstException(Exception):
    message: str
    position: Union[Token, tree.Tree]

    def __init__(self, message: str, position: Union[Token, tree.Tree]):
        Exception.__init__(self)

        self.message = message
        self.position = position


class Value(ABC):
    ty: Type

    def __init__(self, ty: Type):
        self.ty = ty


class Variable(Value, ABC):
    ast: Token
    name: str

    ir_value: Union[None, ir.GlobalVariable, ir.AllocaInstr]

    def __init__(self, ast: Token):
        super().__init__(Type[ast.type])

        self.id = None
        self.name = ast.value
        self.ir_value = None

    def build(self, builder: ir.IRBuilder):
        # If we are inside a block, this is a local context
        # This means we can generate this via alloca
        if builder.block:
            self.ir_value = builder.alloca(self.ty.lower(),
                                           name=f"{builder.block.name}.{self.name}")
        else:
            # This is a global variable
            # Create the symbol
            module: ir.Module = builder.module
            self.ir_value = ir.GlobalVariable(module, self.ty.lower(), self.name)


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
        self.children = []
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


class ContextBuilder:
    loop_labels: Optional[Tuple[ir.Block, ir.Block]]
    context: Context

    def __init__(self):
        self.loop_labels = None
        self.context = Context(None)

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

    def find(self, name: str) -> Optional[Variable]:
        return self.context.find(name)
