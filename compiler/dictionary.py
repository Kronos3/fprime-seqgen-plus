from typing import Dict, Union

from xml.etree import ElementTree

from compiler.type import Type


class Dictionary:
    enums: Dict[str, 'Dictionary.Enum']
    commands: Dict[str, 'Dictionary.Command']

    class Enum:
        name: str
        items: Dict[str, int]

        def __init__(self, e_xml: ElementTree.Element):
            self.name = e_xml.attrib["type"]
            self.items = {}

            for i in e_xml:
                self.items[i.attrib["name"]] = int(i.attrib["value"])

        def __repr__(self):
            return self.name

    class Arg:
        name: str
        type: Union['Dictionary.Enum', Type]

        _len: int

        def __init__(self, d: 'Dictionary', a_xml: ElementTree.Element):
            self.name = a_xml.attrib["name"]
            type_name = a_xml.attrib["type"]
            try:
                self.type = Type[type_name.lower()]
            except KeyError:
                self.type = d.enums[type_name]

        def __repr__(self):
            return f"{self.name}: {repr(self.type)}"

    class Command:
        component: str
        mnemonic: str
        opcode: int

        args: Dict[str, 'Dictionary.Arg']

        def __init__(self, d: 'Dictionary', c_xml: ElementTree.Element):
            self.component = c_xml.attrib["component"]
            self.mnemonic = c_xml.attrib["mnemonic"]
            self.opcode = int(c_xml.attrib["opcode"], 16)

            self.args = {}
            for e in c_xml.iter("arg"):
                a = Dictionary.Arg(d, e)
                self.args[a.name] = a

        @property
        def name(self) -> str:
            return f"{self.component}.{self.mnemonic}"

        def __repr__(self):
            return self.name + " " + " ".join(repr(x) for x in self.args.values())

    def __init__(self, dictionary_file):
        tree = ElementTree.parse(dictionary_file)
        root = tree.getroot()

        self.enums = {}
        self.commands = {}

        # Enums then commands
        for e_xml in root.find("enums").iter("enum"):
            e = Dictionary.Enum(e_xml)
            self.enums[e.name] = e

        for c_xml in root.find("commands").iter("command"):
            c = Dictionary.Command(self, c_xml)
            self.commands[c.name] = c
