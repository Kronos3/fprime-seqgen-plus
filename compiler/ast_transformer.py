from typing import List
from dataclasses import dataclass

import lark
from lark import ast_utils, Transformer, v_args
from lark.tree import Meta


#
#   Define AST
#
class _Ast(ast_utils.Ast):
    # This will be skipped by create_transformer(), because it starts with an underscore
    pass


class _Statement(_Ast):
    # This will be skipped by create_transformer(), because it starts with an underscore
    pass


@dataclass
class Value(_Ast, ast_utils.WithMeta):
    """Uses WithMeta to include line-number metadata in the meta attribute"""
    meta: Meta
    value: object


@dataclass
class CName(_Ast):
    name: str

@dataclass
class Suite(_Ast, ast_utils.AsList):
    # Corresponds to code_block in the grammar
    statements: List[_Statement]


@dataclass
class If(_Statement):
    cond: Value
    then: Suite


@dataclass
class SetVar(_Statement):
    # Corresponds to set_var in the grammar
    name: str
    value: Value


@dataclass
class Print(_Statement):
    value: Value


# noinspection PyPep8Naming
class ToAst(Transformer):
    # Define extra transformation functions, for rules that don't correspond to an AST class.

    @staticmethod
    def ESCAPED_STRING(s):
        # Remove quotation marks
        return s[1:-1]

    @staticmethod
    def INT(n):
        # Remove quotation marks
        return int(n)

    @staticmethod
    def HEX_NUMBER(n):
        return int(n, 16)

    @staticmethod
    def FLOAT(s: lark.Token):
        # Remove quotation marks
        return float(s)

    @staticmethod
    def lit_true(_):
        return True

    @staticmethod
    def lit_false(_):
        return False

    @v_args(inline=True)
    def start(self, x):
        return x
