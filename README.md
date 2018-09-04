# glulx-assemble

glulx-assemble is an assembler for creating glulx program files. It is a
re-imagined version of my previous project
[GGASM](https://github.com/GrenDrake/ggasm) written using C99. The main intent
behind this program is to serve as a backend that assembles data output by other
applications, I have tried to design it in such a way that it can be used to
create programs in pure assembly as well.


## Usage

glulx-assemble is a command line program with two optional arguments: the name
of a source file to assemble and the name of the output file. By default these
are named *input.ga* and *output.ulx* respectively.

```
glulx-assemble <input file> <output file>
```


## Compiling

glulx-assemble is written in standard C99; it should be possible to compile it
with any compliant compiler. It is packaged using a simple makefile for
building. There are no external dependencies.


## Source File Syntax

| Directive | Paramaters | Description |
| --- | --- | --- |
| [identifier: label name]: | | create a label with the given name saving this code position |
| .define         | [string: name] [int: value]     | define a constant identifier |
| .include        | [string: filename]              | include another file at this location; no multiple inclusion guard |
| .include_binary | [string: filename]              | include raw binary data |
| .byte           | ([int:   value])+               | series of one-byte values |
| .short          | ([int:   value])+               | series of two-byte values |
| .stack_size     | [int:    stack size]            | set the stack size for the glulx VM |
| .string         | [string: text]                  | ASCII string |
| .word           | ([int:   value])+               | series of four-byte values |
| .zero           | [int:    bytes]                 | set number of zero-value bytes |


## License

glulx-assemble is released under the MIT license. Program files created by
glulx-assemble are not covered by this license and may be licensed as the
copyright holder desires.
