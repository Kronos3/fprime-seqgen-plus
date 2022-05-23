## Seqgen plus architecture

The virtual machine target binary is organized in
the following way.

1. Symbol Table (global variable declarations)
2. Constant Data (for string literals etc.)
3. Text - serialized code blocks

### Symbol table

Unlike in an ELF binary, all global symbols will sit in an uninitialized
section and be initialized at runtime using bytes from the data section.

The symbol table is laid out as follows:

| Size | Description  | Type          |
|------|--------------|---------------|
| 4    | Symbol count | U32           |
| xx   | Symbols      | <symbol> list |

Each symbol will have the following format:

| Size | Description | Type   |
|------|-------------|--------|
| 2    | Variable id | U16    |
| 1    | Type id     | U8     |

### Constant Data

Constant data is a block of variable size will read-only data
This data may be referenced by a load-constant instruction
and an address to the data in this memory block.

This may be used for string literals or raw bytes to load.

### Text

Text is a set of instructions. Every instruction will start with a
2-byte op-code. Different classes of instructions may have different
lengths. Branch instructions should reference addresses relative to the
start of the file rather than the start of the text section.