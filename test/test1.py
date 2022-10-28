from lark import Lark, ast_utils
from pathlib import Path

import compiler.ast_transformer

from lark.indenter import Indenter


class TreeIndenter(Indenter):
    NL_type = '_NEWLINE'
    OPEN_PAREN_types = []
    CLOSE_PAREN_types = []
    INDENT_type = '_INDENT'
    DEDENT_type = '_DEDENT'
    tab_len = 8


def main():
    curr_path = Path(__file__).parent

    parser = Lark((curr_path.parent / "ast/grammar.lark").open("r").read(), parser="lalr", postlex=TreeIndenter())

    tree = parser.parse((curr_path / "test1.seq").open("r").read())
    print(tree.pretty())

    transformer = ast_utils.create_transformer(compiler.ast_transformer, compiler.ast_transformer.ToAst())
    ast = transformer.transform(tree)

    print(ast)

    return 0


if __name__ == "__main__":
    exit(main())
