from lark import Lark
from pathlib import Path


def main():
    curr_path = Path(__file__).parent

    print("Parsing grammar.lark")
    parser = Lark((curr_path / "grammar.lark").open("r").read(), parser="lalr")

    print("Serializing parser to grammar.obj")
    parser.save((curr_path / "grammar.obj").open("wb+"))

    return 0


if __name__ == "__main__":
    exit(main())
