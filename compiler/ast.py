import enum
from abc import ABC, abstractmethod
from pathlib import Path
from typing import Dict, Callable, Union, Optional, List, Tuple, cast

from lark import tree, Lark, Token
from lark.indenter import Indenter

from compiler.common import AstException, ContextBuilder, Variable, Value
from compiler.type import Type

from llvmlite import ir

Location = Union[tree.Tree, Token]


def _label_suffix(label: str, suffix: str) -> str:
    if len(label) > 50:
        nhead = 25
        return ''.join([label[:nhead], '..', suffix])
    else:
        return label + suffix


class Ast(Value, ABC):
    ast: tree.Tree
    ast_builder: 'AstBuilder'

    def __init__(self,
                 ty: Type,
                 ast: tree.Tree,
                 ast_builder: 'AstBuilder'):
        Value.__init__(self, ty)
        self.ast = ast
        self.ast_builder = ast_builder

    @abstractmethod
    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        """
        Build IR given an AST
        :param builder: IR builder
        :return: None
        """


class AstFile:
    filename: str
    contents: str
    lines: List[str]

    def __init__(self, filename):
        filename = Path(filename)
        self.filename = filename.name
        with filename.open("r") as f:
            self.contents = f.read()
            self.lines = self.contents.split("\n")


class AstMessage:
    NOTE = 36, "note"  # cyan
    WARNING = 33, "warning"  # purple
    ERROR = 31, "error"  # red

    file: AstFile
    message: AstException
    message_type: Tuple[int, str]

    def __init__(self,
                 file: AstFile,
                 message_type: Tuple[int, str],
                 message: AstException):
        self.file = file
        self.message = message
        self.message_type = message_type

    @staticmethod
    def format_color(color_number: int, message: str) -> str:
        return f"\033[1;{color_number}m{message}\033[1;0m"

    def __str__(self):
        message = "\033[1m%s:%d:%d %s: %s" % (
            self.file.filename,
            self.message.position.line,
            self.message.position.column,
            self.format_color(*self.message_type),
            self.message,
        )

        num_spaces = 6 - len(str(self.message.position.line))
        indent_1 = " " * num_spaces
        indent_2 = " " * 6

        line = self.file.lines[self.message.position.line - 1]
        formatted_line = line[0:self.message.position.column - 1] + self.format_color(
            self.message_type[0],
            line[self.message.position.column - 1:self.message.position.end_column - 1]
        ) + line[self.message.position.end_column - 1:]

        line_1 = f"{indent_1}{self.message.position.line} | {formatted_line}"

        underline = self.format_color(self.message_type[0],
                                      "^" + "~" * (self.message.position.end_column - self.message.position.column - 1))
        line_2 = f"{indent_2} | " + (" " * (self.message.position.column - 2)) + underline

        return f"{message}\n{line_1}\n{line_2}\n"


class AstBuilder:
    class TreeIndenter(Indenter):
        NL_type = '_NEWLINE'
        OPEN_PAREN_types = []
        CLOSE_PAREN_types = []
        INDENT_type = '_INDENT'
        DEDENT_type = '_DEDENT'
        tab_len = 8

    context_builder: ContextBuilder
    module: ir.Module
    builder: ir.IRBuilder

    file: AstFile
    ast: tree.Tree

    has_errors: bool
    ast_messages: List[AstMessage]

    function_context: Optional['FunctionDecl']

    _AST_HANDLERS: Dict[str, Callable[[tree.Tree, 'AstBuilder'], Ast]]

    def __init__(self, file: AstFile, use_cached: bool = True):
        self.context_builder = ContextBuilder()
        self.module = ir.Module(file.filename)
        self.builder = ir.IRBuilder()

        self.file = file
        if use_cached:
            raise NotImplementedError
        else:
            curr_path = Path(__file__).parent
            parser = Lark((curr_path.parent / "ast/grammar.lark").open("r").read(),
                          parser="lalr", postlex=AstBuilder.TreeIndenter())
            self.ast = parser.parse(self.file.contents)

        self.ast_messages = []
        self.function_context = None

        self._AST_HANDLERS = {
            "decl_assign": DeclAssign,
            "variable_decl": VariableDecl,
            "assign": Assign,
            "CNAME": VarExpr,
            "lt": BinaryExpr,
            "gt": BinaryExpr,
            "le": BinaryExpr,
            "ge": BinaryExpr,
            "eq": BinaryExpr,
            "neq": BinaryExpr,
            "mul": BinaryExpr,
            "div": BinaryExpr,
            "add": BinaryExpr,
            "sub": BinaryExpr,
            "log_and": BinaryExpr,
            "log_or": BinaryExpr,
            "lit_float": ConstExpr,
            "lit_int": ConstExpr,
            "lit_string": ConstExpr,
            "lit_true": ConstExpr,
            "lit_false": ConstExpr,

            "function_decl": FunctionDecl,
            "start": Suite,
            "suite": Suite,
            "while_stmt": WhileStmt,
            "if_stmt": IfStmt,
            "continue": ContinueStmt,
            "break": BreakStmt,
            "pass": PassStmt,
        }

    def emit_error(self, message: AstException):
        self.has_errors = True
        self.ast_messages.append(AstMessage(self.file, AstMessage.ERROR, message))

    def emit_warning(self, message: AstException):
        self.ast_messages.append(AstMessage(self.file, AstMessage.WARNING, message))

    def emit_note(self, message: AstException):
        self.ast_messages.append(AstMessage(self.file, AstMessage.NOTE, message))

    def put_messages(self) -> bool:
        for message in self.ast_messages:
            print(str(message))
        return self.has_errors

    def lower(self, ast: tree.Tree) -> Ast:
        return self._AST_HANDLERS[ast.data](ast, self)


class ConstExpr(Ast):
    value: Union[float, int, str, bool]

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        ty: Type

        if ast.data == "lit_float":
            self.value = float(ast.children[0].value)
            ty = Type.f64
        elif ast.data == "lit_int":
            self.value = int(ast.children[0].value)
            ty = Type.i32
        elif ast.data == "lit_string":
            self.value = str(ast.children[0].value)
            ty = Type.string
        elif ast.data == "lit_true":
            self.value = True
            ty = Type.boolean
        elif ast.data == "lit_false":
            self.value = False
            ty = Type.boolean
        else:
            assert False, ast.pretty()

        Ast.__init__(self, ty, ast, ast_builder)

    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        return ir.Constant(self.ty.lower(), self.value)


class VariableDecl(Ast):
    token: Token
    variable: Variable

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        self.token: Token = ast.children[0]
        super().__init__(Type[ast.children[1].data], ast, ast_builder)

    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        # Add the variable to the module
        # ...or create an alloca for locals
        self.variable.build(builder)
        return None  # decls should not return values


class DeclAssign(VariableDecl):
    assign: Ast

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        VariableDecl.__init__(self, ast.children[0], ast_builder)
        self.assign = ast_builder.lower(ast.children[1])

    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        # Build the decl first
        VariableDecl.build(self, builder)

        # Store the value into the variable
        builder.store(self.assign.build(builder), self.variable.ir_value)

        return None


class VarExpr(Ast):
    variable: Optional[Variable]

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        ast_token: Token = cast(Token, ast)
        self.variable = ast_builder.context_builder.find(ast_token.value)

        # Search for a previous decl in the context
        # If not defined, default to int
        if self.variable:
            Ast.__init__(self, self.variable.ty, ast, ast_builder)
        else:
            self.ast_builder.emit_error(AstException("", ast))
            Ast.__init__(self, Type.i32, ast, ast_builder)

    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        # Load the value from memory
        return builder.load(self.variable.ir_value)


class Assign(VarExpr):
    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        VarExpr.__init__(self, ast, ast_builder)
        self.assign = ast_builder.lower(ast.children[1])

    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        value_to_assign = self.assign.build(builder)
        builder.store(self.variable.ir_value, value_to_assign)
        return value_to_assign


class BinaryExpr(Ast):
    # noinspection PyArgumentList
    class Opcode(enum.IntEnum):
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

        # Arithmetic binary operations
        DIV = enum.auto()
        MUL = enum.auto()
        ADD = enum.auto()
        SUB = enum.auto()

        @staticmethod
        def cmp_wrapper(operator: str):
            def cmp_runner(builder: ir.IRBuilder, lhs: Ast, rhs: Ast):
                if lhs.ty.is_signed:
                    return builder.icmp_signed(operator, lhs.build(builder), rhs.build(builder))
                elif lhs.ty.is_unsigned:
                    return builder.icmp_unsigned(operator, lhs.build(builder), rhs.build(builder))
                elif lhs.ty.is_floating:
                    return builder.fcmp_ordered(operator, lhs.build(builder), rhs.build(builder))
                else:
                    assert False, lhs.ty

            return cmp_runner

        @staticmethod
        def arith_wrapper(fcall: Callable[[ir.IRBuilder, ir.Value, ir.Value],
                                          ir.instructions.Instruction],
                          icall: Callable[[ir.IRBuilder, ir.Value, ir.Value],
                                          ir.instructions.Instruction]):
            def arith_runner(builder: ir.IRBuilder, lhs: Ast, rhs: Ast):
                if lhs.ty.is_integer:
                    return icall(builder, lhs.build(builder), rhs.build(builder))
                elif lhs.ty.is_floating:
                    return fcall(builder, lhs.build(builder), rhs.build(builder))
                else:
                    assert False, lhs.ty

            return arith_runner

        @staticmethod
        def div_helper(builder: ir.IRBuilder, lhs: Ast, rhs: Ast):
            if lhs.ty.is_signed:
                return builder.sdiv(lhs.build(builder), rhs.build(builder))
            elif lhs.ty.is_unsigned:
                return builder.udiv(lhs.build(builder), rhs.build(builder))
            elif lhs.ty.is_floating:
                return builder.fdiv(lhs.build(builder), rhs.build(builder))
            else:
                assert False, lhs.ty

    lhs: Ast
    rhs: Ast

    operation: str
    opcode: Opcode

    __LOWERING__: Dict['BinaryExpr.Opcode', Callable[[ir.IRBuilder, Ast, Ast],
                                                     ir.instructions.Instruction]] = {
        Opcode.LT: Opcode.cmp_wrapper("<"),
        Opcode.GT: Opcode.cmp_wrapper(">"),
        Opcode.LE: Opcode.cmp_wrapper("<="),
        Opcode.GE: Opcode.cmp_wrapper(">="),
        Opcode.EQ: Opcode.cmp_wrapper("=="),
        Opcode.NEQ: Opcode.cmp_wrapper("!="),
        Opcode.ADD: Opcode.arith_wrapper(ir.IRBuilder.add, ir.IRBuilder.fadd),
        Opcode.SUB: Opcode.arith_wrapper(ir.IRBuilder.sub, ir.IRBuilder.fsub),
        Opcode.MUL: Opcode.arith_wrapper(ir.IRBuilder.mul, ir.IRBuilder.fmul),
        Opcode.DIV: Opcode.div_helper,
    }

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        self.lhs = ast_builder.lower(ast.children[0])
        self.rhs = ast_builder.lower(ast.children[1])

        self.operation = ast.data
        self.opcode = BinaryExpr.Opcode[self.operation]

        super().__init__(self.lhs.ty, ast, ast_builder)

    def build(self, builder: ir.IRBuilder) -> Optional[ir.Value]:
        try:
            return BinaryExpr.__LOWERING__[self.opcode](builder, self.lhs, self.rhs)
        except Exception as e:
            raise AstException(str(e), self.ast) from e


class Suite(Ast):
    statements: List[Ast]

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_builder)

        self.statements = []
        for s in ast.children:
            self.statements.append(ast_builder.lower(s))

    def build(self, builder: ir.IRBuilder):
        self.ast_builder.context_builder.push_context()
        for s in self.statements:
            # We are able to emit one error per line
            try:
                s.build(builder)
            except AstException as e:
                self.ast_builder.emit_error(e)
            except Exception as e:
                # This is a generic uncaught exception
                # Annotate the entire line
                self.ast_builder.emit_error(AstException(str(e), s.ast))

        self.ast_builder.context_builder.pop_context()
        return None


class WhileStmt(Ast):
    expr: Ast
    loop: Ast

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_builder)

        self.expr = ast_builder.lower(ast.children[0])
        self.loop = ast_builder.lower(ast.children[1])

    def build(self, builder: ir.IRBuilder):
        bb = builder.block
        bb_while_check = builder.append_basic_block(name=_label_suffix(bb.name, '.while_c'))
        bb_while_body = builder.append_basic_block(name=_label_suffix(bb.name, '.while_b'))
        bb_endwhile = builder.append_basic_block(name=_label_suffix(bb.name, '.endwhile'))

        old_loops = self.ast_builder.context_builder.loop_labels

        self.ast_builder.context_builder.loop_labels = (
            bb_while_check, bb_endwhile
        )

        # Build the conditional block
        with bb_while_check:
            evaluated_condition = self.expr.build(builder)
            loop_br = builder.cbranch(evaluated_condition, bb_while_body, bb_endwhile)

            # Loops... uh... loop?
            # Assume they loop, the scheduler will generate nicer code :)
            loop_br.set_weights([99, 1])

        # Build the loop body
        with bb_while_body:
            self.loop.build(builder)

            # Loop-de-loop
            builder.branch(bb_while_check)

        # Restore the old loop labels
        self.ast_builder.context_builder.loop_labels = old_loops

        return None


class IfStmt(Ast):
    class IfElifClause:
        expr: Ast
        if_true: Ast

        def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
            self.expr = ast_builder.lower(ast.children[0])
            self.if_true = ast_builder.lower(ast.children[1])

    if_clause: IfElifClause
    elif_clauses: List[IfElifClause]
    else_clause: Optional[Ast]

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_builder)

        self.if_clause = IfStmt.IfElifClause(ast.children[0], ast_builder)
        self.elif_clauses = []

        for i in ast.children[1:]:
            if i.data == "elif_clause":
                assert len(i.children) == 2
                self.elif_clauses.append(IfStmt.IfElifClause(ast, ast_builder))
            else:
                assert i.data == "else_clause"
                break

        if ast.children[-1].data == "else_clause":
            self.else_clause = ast_builder.lower(ast.children[-1])
        else:
            self.else_clause = None

    def build(self, builder: ir.IRBuilder) -> Optional[Value]:
        bb = builder.block
        bbif = builder.append_basic_block(name=_label_suffix(bb.name, '.if'))
        bbelse = builder.append_basic_block(name=_label_suffix(bb.name, '.else'))
        bbend = builder.append_basic_block(name=_label_suffix(bb.name, '.endif'))

        builder.cbranch(self.if_clause.expr.build(builder), bbif, bbelse)

        with bbif:
            self.if_clause.if_true.build(builder)
            builder.branch(bbend)

        for elif_stmt in self.elif_clauses:
            bb = bbelse
            with bbelse:
                bbif = builder.append_basic_block(name=_label_suffix(bb.name, '.if'))
                bbelse = builder.append_basic_block(name=_label_suffix(bb.name, '.else'))

                builder.cbranch(elif_stmt.expr.build(builder), bbif, bbelse)

                with bbif:
                    elif_stmt.if_true.build(builder)
                    builder.branch(bbend)

        if self.else_clause is not None:
            with bbelse:
                self.else_clause.build(builder)
                builder.branch(bbend)

        builder.position_at_end(bbend)
        return None


class FunctionDecl(Ast):
    name: str
    return_type: Type
    args: List[VariableDecl]

    suite: Ast
    function: Optional[ir.types.FunctionType]

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_builder)

        if self.ast_builder.function_context is not None:
            self.ast_builder.emit_error(AstException(
                "Nested function declarations are not allowed", self.ast
            ))
            self.ast_builder.emit_note(AstException(
                "Last declaration was here", self.ast_builder.function_context.ast
            ))

        # Lower the code ast even if there is an error for nested declarations
        self.name = self.ast.children[0].value
        self.args = []

        if len(ast.children) == 3:
            # No arguments
            self.return_type = Type[ast.children[2].data]
            self.suite = ast_builder.lower(ast.children[3])
        else:
            assert len(ast.children) == 4
            for arg in ast.children[1]:
                self.args.append(VariableDecl(arg, ast_builder))
            self.return_type = Type[ast.children[3].data]
            self.suite = ast_builder.lower(ast.children[4])

        self.function = None

    def build(self, builder: ir.IRBuilder) -> Optional[Value]:
        self.function = ir.types.FunctionType(
            self.return_type.lower()
        )

        return None


class ReturnStmt(Ast):
    expr: Optional[Ast]

    def __init__(self, ast: tree.Tree, ast_build: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_build)

        if self.ast.children:
            self.expr = self.ast.children[0]
        else:
            self.expr = None

    def build(self, builder: ir.IRBuilder) -> Optional[Value]:
        if self.expr:
            builder.ret(self.expr.build(builder))
        else:
            builder.ret_void()

        return None


class ContinueStmt(Ast):
    def __init__(self, ast: tree.Tree, ast_build: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_build)

    def build(self, builder: ir.IRBuilder) -> Optional[Value]:
        bb_while_check, _ = self.ast_builder.context_builder.loop_labels
        builder.branch(bb_while_check)
        return None


class BreakStmt(Ast):
    def __init__(self, ast: tree.Tree, ast_build: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_build)

    def build(self, builder: ir.IRBuilder) -> Optional[Value]:
        _, bb_while_end = self.ast_builder.context_builder.loop_labels
        builder.branch(bb_while_end)
        return None


class PassStmt(Ast):
    def __init__(self, ast: tree.Tree, ast_build: AstBuilder):
        Ast.__init__(self, Type.void, ast, ast_build)

    def build(self, builder: ir.IRBuilder) -> Optional[Value]:
        return None
