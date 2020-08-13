# Demo Programs

Several demo programs are included with the glulx-assemble distribution.
These are intended to serve as examples of how to write programs and use the assembler's capabilities; they are generally not useful otherwise.

## [minimal.ga]

The absolute minimum code required to assemble a program.
Does absolutely nothing.

## [basic.ga]

A very basic demo that sets up GLK, verifies the integrity of the program file, then prints a number of strings.
Intended as a more functional minimal example.

## [complex.ga]

A more complex demo making use of a larger range of the assembler's features.
Currently very unfocused and is likely to be replaced at some point in the future by more specific advanced examples.

## [expressions.ga]

Tests operand expressions.
This is intended as an assembler test more than a demo.

## [model.ga]

A basic port of Andrew Plotkin's "model.c" demo for GLK.
Currently missing the YADA verb, but otherwise complete.

## [glk.ga]

Include file containing numerous GLK-related constants, including function selectors.
Most programs will want to include this file or an equivalent.

## [mountain.ga], [gamesys.ga]

Partial implementation of a very basic adventure game and library.


[minimal.ga]: ../demos/minimal.ga "Minimal buildable program"

[basic.ga]: ../demos/basic.ga "Basic example program"

[complex.ga]: ../demos/complex.ga "Complex example program"

[expressions.ga]: ../demos/expressions.ga "Test of operand expressions"

[glk.ga]: ../demos/glk.ga "Include file for GLK constants"

[model.ga]: ../demos/model.ga "Port of model.c"

[mountain.ga]: ../demos/mountain.ga "A very basic adventure game"

[gamesys.ga]: ../demos/gamesys.ga "System library for mountain.ga"

