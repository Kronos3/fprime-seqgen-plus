# Seqgen plus
Seqgen plus is a complete rework of the FPrime sequence schema.
This program looks to solve the deficiencies of an FPrime
sequence and pull basic logic from the C++ layer to the
sequence layer allowing for more versatile routines.

## Basic design
Sequences are now a sequence of instructions where some
instructions may be command dispatches. A simple virtual
machine is required to execute instructions on the hardware
and can be implemented with a drop in replacement to the
FPrime Sequencer.

Most of the work is left to the compiler to interpret a
strongly typed Python-like scripting language into a
binary target for the FPrime virtual machine.
