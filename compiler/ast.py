from abc import ABC, abstractmethod
from pathlib import Path
from typing import Dict, Callable, Union, Optional, List, Tuple

from lark import tree, Lark
from lark.indenter import Indenter

from compiler import ir, instructions
from compiler.common import Value, AstException
from compiler.type import Type, INTEGRAL_TYPES


Location = Union[tree.Tree, tree.Token]


class Ast(ABC):
    ast: tree.Tree
    ast_builder: 'AstBuilder'

    def __init__(self, ast: tree.Tree,
                 ast_builder: 'AstBuilder'):
        self.ast = ast
        self.ast_builder = ast_builder

    @abstractmethod
    def build(self, builder: ir.Builder) -> Optional[Value]:
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

    builder: ir.Builder

    file: AstFile
    ast: tree.Tree

    has_errors: bool
    ast_messages: List[AstMessage]

    function_context: Optional['FunctionDecl']

    _AST_HANDLERS: Dict[str, Callable[[tree.Tree, 'AstBuilder'], Ast]]

    def __init__(self, file: AstFile, use_cached: bool = True):
        self.builder = ir.Builder()
        self.file = file
        if use_cached:
            raise NotImplementedError
        else:
            curr_path = Path(__file__).parent.parent
            parser = Lark((curr_path.parent / "ast/grammar.lark").open("r").read(),
                          parser="lalr", postlex=AstBuilder.TreeIndenter())
            self.ast = parser.parse(self.file.contents)

        self.ast_messages = []
        self.function_context = None

        self._AST_HANDLERS = {
            "variable_decl": VariableDecl,
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

            "suite": Suite,
            "while_stmt": WhileStmt,
            "if_stmt": IfStmt
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


class VariableDecl(Ast):
    token: tree.Token
    variable: ir.Variable

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        super().__init__(ast, ast_builder)

        self.token: tree.Token = ast.children[0]
        self.variable = ir.Variable(self.token)

    def build(self, builder: ir.Builder) -> Optional[Value]:
        already_exists = builder.context.find(self.token.value)
        assert already_exists is None, self.token.value

        # Variable decl should not return a value
        builder.context.variables[self.token.value] = self.variable
        return None


class BinaryExpr(Ast):
    lhs: Ast
    rhs: Ast

    operation: str
    opcode: instructions.Opcode

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        super().__init__(ast, ast_builder)

        self.lhs = ast_builder.lower(ast.children[0])
        self.rhs = ast_builder.lower(ast.children[1])

        self.operation = ast.data
        self.opcode = instructions.Opcode[self.operation]

        assert self.opcode in instructions.LOGICAL_BINARY or \
               self.opcode in instructions.ARITHMETIC_BINARY, self.opcode

    def _validate_logical(self, lhs: instructions.Value, rhs: instructions.Value):
        # Now validate the input types
        if lhs.ty in INTEGRAL_TYPES and rhs.ty in INTEGRAL_TYPES:
            # These can always be compared
            pass
        elif Type.bool in (lhs.ty, rhs.ty):
            if lhs.ty != rhs.ty:
                raise AstException("bool only supports operations with other bools", self.ast)
        elif Type.string in (lhs.ty, rhs.ty):
            if self.opcode not in (instructions.Opcode.EQ, instructions.Opcode.NEQ):
                raise AstException("string only supports '==' and '!='", self.ast)
            if lhs.ty != rhs.ty:
                raise AstException("string only supports operations with other string", self.ast)
        else:
            assert Type.bytes in (lhs.ty, rhs.ty), (lhs.ty, rhs.ty)
            # TODO(tumbar) Do we want to annotate left or right (or just the whole thing)
            raise AstException("Cannot perform logical operation on bytes", self.ast)

    def _validate_arithmetic(self, lhs: instructions.Value, rhs: instructions.Value):
        if lhs.ty in INTEGRAL_TYPES and rhs.ty in INTEGRAL_TYPES:
            # These can always be operated on
            pass
        else:
            raise AstException("Only integral types are support for arithmetic operations", self.ast)

    def build(self, builder: ir.Builder) -> Optional[Value]:
        reg_l = self.lhs.build(builder).as_register(builder)
        reg_r = self.rhs.build(builder).as_register(builder)

        # Validate generated types
        if self.opcode in instructions.LOGICAL_BINARY:
            self._validate_logical(reg_l, reg_r)
        else:
            assert self.opcode in instructions.ARITHMETIC_BINARY
            self._validate_arithmetic(reg_l, reg_r)

        instr: instructions.Instruction
        try:
            instr = instructions.BinaryOp(self.opcode, reg_l, reg_r)
        except Exception as e:
            ast_exception = AstException(str(e), self.ast)
            self.ast_builder.emit_error(ast_exception)
            raise ast_exception

        return builder.append(instr)


class ConstExpr(ir.Constant, Ast):
    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, ast, ast_builder)

        value: Union[str, float, int, bool]
        ty: Type

        if ast.data == "lit_float":
            value = float(ast.children[0].value)
            ty = Type.f64
        elif ast.data == "lit_int":
            value = int(ast.children[0].value)
            ty = Type.i32
        elif ast.data == "lit_string":
            value = str(ast.children[0].value)
            ty = Type.string
        elif ast.data == "lit_true":
            value = True
            ty = Type.bool
        elif ast.data == "lit_false":
            value = False
            ty = Type.bool
        else:
            assert False, ast.pretty()

        ir.Constant.__init__(self, value, ty)

    def build(self, builder: ir.Builder) -> Optional[Value]:
        # Strings need to be stored in constant data
        if self.ty == Type.string:
            builder.const_data.append(self)

        return self


class VarExpr(Ast):
    variable_name: str

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, ast, ast_builder)

        ast: tree.Token
        self.variable_name = ast.value

    def build(self, builder: ir.Builder) -> Optional[Value]:
        v = builder.context.find(self.variable_name)
        assert v is not None, self.variable_name
        return v


class Suite(Ast):
    statements: List[Ast]

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, ast, ast_builder)

        self.statements = []
        for s in ast.children:
            self.statements.append(ast_builder.lower(s))

    def build(self, builder: ir.Builder) -> Optional[Value]:
        builder.push_context()
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

        builder.pop_context()
        return None


class WhileStmt(Ast):
    expr: Ast
    loop: Ast

    def __init__(self, ast: tree.Tree, ast_builder: AstBuilder):
        Ast.__init__(self, ast, ast_builder)

        self.expr = ast_builder.lower(ast.children[0])
        self.loop = ast_builder.lower(ast.children[1])

    def build(self, builder: ir.Builder) -> Optional[Value]:
        loop_start = builder.create_label()
        loop_end = builder.create_label()

        loop_block = builder.new_block(chain_block=True)

        # Hook the start to the start of the loop block
        loop_start.hook(loop_block)

        old_labels = builder.loop_labels
        builder.loop_labels = loop_start, loop_end

        # Perform the expression and store it in a register
        expr_reg = self.expr.build(builder).as_register(builder)

        if expr_reg.ty != Type.bool:
            self.ast_builder.emit_warning(AstException("Expression does not return bool, truncating to bool...",
                                                       self.ast))

        # Check if the register is true or false
        # Branch to the end if the condition is false
        builder.append(instructions.Branch(expr_reg, loop_end, branch_if=False))

        # Place the loop contents in this block
        self.loop.build(builder)

        # Automatically loop back to the start of the loop
        # This will recheck the condition
        builder.append(instructions.Jump(loop_start))

        # This is block that executes after the loop finishes
        after_loop_block = builder.new_block(False)
        loop_end.hook(after_loop_block)

        # Restore the old loop labels
        builder.loop_labels = old_labels

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
        Ast.__init__(self, ast, ast_builder)

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

    def build(self, builder: ir.Builder) -> Optional[Value]:
        next_label = builder.create_label()

        if_reg = self.if_clause.expr.build(builder).as_register(builder)

        if if_reg.ty != Type.bool:
            self.ast_builder.emit_warning(AstException("Expression does not return bool, truncating to bool...",
                                                       self.if_clause.expr.ast))

        builder.append(instructions.Branch(if_reg, next_label, branch_if=False))

        if_block = builder.new_block(chain_block=True)
        self.if_clause.if_true.build(builder)

        if_blocks = [if_block]

        for elif_stmt in self.elif_clauses:
            # Hook the false condition of the last statement here
            elif_block = builder.new_block(chain_block=False)
            next_label.hook(elif_block)
            if_blocks.append(elif_block)

            next_label = builder.create_label()
            elif_reg = elif_stmt.expr.build(builder).as_register(builder)

            if elif_reg.ty != Type.bool:
                self.ast_builder.emit_warning(AstException("Expression does not return bool, truncating to bool...",
                                                           elif_stmt.expr.ast))

            builder.append(instructions.Branch(elif_reg, next_label, branch_if=False))

            # Add the actual stmt instructions
            elif_stmt.if_true.build(builder)

        if self.else_clause is not None:
            else_block = builder.new_block(chain_block=False)
            next_label.hook(else_block)
            if_blocks.append(else_block)

            next_label = builder.create_label()

            # Add the actual stmt instructions
            self.else_clause.build(builder)

        after_if_block = builder.new_block(chain_block=False)
        next_label.hook(after_if_block)

        for b in if_blocks:
            b.next = after_if_block

        return None


class FunctionDecl(Ast):
    name: str
    args: List[Tuple[str, VariableDecl]]

    suite: Ast

    def __init__(self, ast: tree.Tree, ast_build: AstBuilder):
        Ast.__init__(self, ast, ast_build)

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


    def build(self, builder: ir.Builder) -> Optional[Value]:
        pass



class ReturnStmt(Ast):
    def __init__(self, ast: tree.Tree, ast_build: AstBuilder):
        Ast.__init__(self, ast, ast_build)

        if ast.children

    def build(self, builder: ir.Builder) -> Optional[Value]:
        pass
