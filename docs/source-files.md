
# Assembly Source Files

Source files are text files encoded in UTF-8 with the ".ga" extension. Some sample source files are in the demos directory; [basic.ga] provides a "hello world" style example. As a special case, the filename can be given as "-" (without the quotes) to read the source file from stdin.

A source file contains the one-line statements used to create the glulx "ulx" file. There are three elements that can occur on a line: a label, a mnemonic or directive, and a comment. If multiple elements are present, they must be in that order.

A label is an identifier followed by a colon. It will create a constant symbol referring to that position in the final file. Typical uses for labels are for use with the various jump opcodes, for saving the location of functions, and for saving the location of data.

This is followed by a mnemonic or directive and its accompanying operands which are described in the section below. Mnemonics are statements that translate directly into glulx bytecode. Directives give an instruction to the assembler and may or may not produce any output.

The last element that can occur on a line is a comment. Comments begin with a semicolon and last until the end of the line. The contents of a comment are completely ignored by the assembler; typically they are used to document the code to make it easier to read in the future.

An example of a source line containing all three elements is included below:

```
start_loop: aloadb a_string, char_num, char_dest   ; load next character from string
```


## Instructions

A mnemonic consists of the mnemonic itself followed by zero or more operands separated by commas. For an index of opcode mnemonics and their expected operands, check the [relevant part of the glulx spec]. Mnemonic operands can take several forms as shown in the table below. Values may also be combined at assembly time to form a single operand using operand expressions.

|          Type          |    Sample    |                                  Description                                   |
|------------------------|--------------|--------------------------------------------------------------------------------|
| Integer literal        | `-42`        |                                                                                |
| Hexadecimal literal    | `$7F`        |                                                                                |
| Floating-point literal | `63.5`       |                                                                                |
| Label name             | `start_loop` |                                                                                |
| Local variable name    | `index`      | The name of a local variable declared in the last encountered function header. |
| Named constant         | `MAX_LENGTH` | Created with the `.define` directive.                                          |

### Operand Expressions

An operand expression is a combination of multiple values using various operators. While the values used do not need to be defined before the expression is encountered, they must have a value known at assemble-time (you can't, for instance, use the contents of a local variable). The supported operators are demonstrated in [expressions.ga]. An example is shown below.

```
streamnum A_NUMBER + 1
.define A_NUMBER 42
```

While an expression can consist of more than two terms, this is not recommended at this time. The order in which the expression is evaluated is currently undefined and will both likely produce unexpected results and change in the future.

### Custom Opcodes

There is a special mnemonic named `opcode` which allows the use of custom opcodes not known to the assembler.  `opcode` should be immediately followed by the opcode number (or a constant holding that number). The world `opcode` may also be immediately followed by `rel` to indicate that the last operand should be treated as a relative value like the [branching opcodes] of glulx.

```
opcode 112 32             ; print a single space character
opcode rel $20 some_label ; jump to "some_label"

.define MY_STREAMCHAR 112
opcode MY_STREAMCHAR 32             ; print a single space character
```


## Directives

Directives provide instructions to the assembler. Many (but not all) produce output. All directives begin with a period.

Two directives are required in any valid glulx-assemble program: `.function` to declare the `start` function and `.end_header` to mark where the header data ends.

### General

**.define**: Defines a symbol with a specified constant value. The value must be a numeric literal or a previously defined constant value.

A few constants are automatically defined. These are *_RAMSTART*, *_EXTSTART*, and *_ENDMEM* which have the same value as header fields of the same name.

```
.define MAX_LENGTH 512
```

**.end_header**: Marks the end of the header data. All information contained in the header will be read-only for the program. The length of the header will be padded with zero bytes to reach a 256 byte boundary.

This may be included at any point in the file, even at the end. However, everything in the file prior to this point will effectively be read-only.

```
.end_header
```

**.extra_memory**: Tells the interpreter to add this much zero-initialized memory to the end of the loaded program. This space is not included in the program file itself and cannot be manipulated by the assembler. It must be a multiple of 256 bytes.

```
.extra_memory 512
```

**.function**: Adds a function header to the output file. The directive may be immediately followed by `stk` to specify that the arguments to this function should be passed on the stack rather than copied into the local variables (see the [glulx spec][glulx spec 6.2] for details). This is followed by a list of the local variable names; the number of names will determine the number of locals for the function.

Every glulx program must create at least one function identified by the label `start`; this will be the entry point of the program. Other functions should generally be identified by appropriate labels as well, though it is not technically required.

```
.function stk a_local_var
.function a_number index
```

**.stack_size**: Sets the stack size the glulx interpreter should use to run this program file. This must be a multiple of 256 bytes and is specified in bytes. If not specified, this will default to a value of 2048 bytes.

```
.stack_size 2048
```

**.string_table**: Tells the assembler to include the Huffman decoding table in this location. This is required in order to be able to display Huffman encoded strings. This directive can be included more than once; the tables will be effectively identical (though the program is permitted to modify one) and the last included table will be set as the default.

```
.string_table
```

### File inclusion

**.include**: Process another file as though its contents occurred at this point in the current file. glulx-assemble will look for included files relative to the current directory, not the directory the current source file is located in. No include guards are used; infinite include loops will freeze or crash the assembler. Included files cannot be read from stdin and the filename "-" is forbidden.

```
.include "glk.ga"
```

**.include_binary**: Includes the raw binary content of another file at the current position of the output file. See the `.include` directive above for general details about the include system.

```
.include "great_art.png"
```

### Raw Data Directives

These directives can be given labels or named constants as well as the various numeric literals for their arguments.

**.byte**, **.short**, **.word**: Include one or more values of the specified length in the output file at the current position. These are 1, 2, and 4 bytes respectively.

```
.byte 54 76 23 81
```

**.cstring**, **.string**: Includes an unencoded, non-unicode string at the current position in the output file. The second version will include the object type byte (0xE0) required for the glulx opcode `streamstr` to be able to output the string.

```
.string "Hello world.\n"
```

**.encoded**: Includes a Huffman encoded string at the current location. In order to actually display an encoded string, the program must include the string decoding table via the .string_table directive.

```
.encoded "Hello Мир.\n"
```

**.pad**: Pads the current length of the output file to the specified amount by adding zero bytes.

```
.pad 256
```

**.unicode**: Includes an unencoded unicode string at the current position. A glulx unicode string is a series of four-byte codepoints.

```
.unicode "こんにちは世界。\n"
```

**.zero**: Include one or more zero bytes in the output file.

```
.zero 54
```

[basic.ga]: ../demos/basic.ga "View source file"
[expressions.ga]: ../demos/expressions.ga "View source file"
[glulx spec 6.2]: https://www.eblong.com/zarf/glulx/glulx-spec_1.html#s.6.2 "Read Glulx official specs on this topic"
[relevant part of the glulx spec]: https://www.eblong.com/zarf/glulx/glulx-spec_2.html "Read Glulx official specs on this topic"
[branching opcodes]: https://www.eblong.com/zarf/glulx/Glulx-Spec.html#opcodes_branch "Branching Opcodes for Glulx"

<!-- EOF -->
