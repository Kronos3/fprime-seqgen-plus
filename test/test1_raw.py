from lark import Lark
from pathlib import Path

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

    ast = parser.parse((curr_path / "test1.seq").open("r").read())
    print(ast.pretty())

    return 0


if __name__ == "__main__":
    exit(main())
