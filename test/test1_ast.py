from pathlib import Path
from compiler import ast


def main():
    curr_path = Path(__file__).parent
    print(__file__, curr_path, curr_path.parent, (curr_path.parent / "ast/grammar.lark"))
    f = ast.AstFile((curr_path / "test1.seq"))
    ast_builder = ast.AstBuilder(f, use_cached=False)

    lowered_ast = ast_builder.lower(ast_builder.ast)
    lowered_ast.build(ast_builder.builder)

    return 0


if __name__ == "__main__":
    exit(main())
