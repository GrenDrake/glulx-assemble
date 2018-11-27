# glulx-assemble

glulx-assemble is an assembler for creating [glulx] program files. It is a re- imagined version of my previous project [GGASM] written using C99. The main intentbehind this program is to serve as a backend that assembles data output by other applications, I have tried to design it in such a way that it can be used to create programs in pure assembly as well.


## License

glulx-assemble is released under the MIT license. Program files created by glulx-assemble are not covered by this license and may be licensed as the copyright holder desires.


## Usage

glulx-assemble is a command line program. It can be run without arguments, in which case it will read from *input.ga* and create *output.ulx*. It can also be passed a number of arguments, as seen in the table below, as well as the names of an input and output file (in that order).

A short header will be added after the standard glulx header that includes the 4-byte string "gasm" and a twelve byte timestamp in the format "YYYYMMDDHHMM". The contents of the timestamp can be customized through the command line to consist of any text up to twelve bytes, including an empty string.

Most of these options are intended for debugging the assembler and will not be generally useful.

|      Argument     |                                               Description                                               |
|-------------------|---------------------------------------------------------------------------------------------------------|
| `-dump-labels`    | Dumps a list of all labels and named constants defined in the program after all assembly was completed. |
| `-dump-patches`   | Dumps a list of all the back-patches used by the assembler in creating the final program file.          |
| `-dump-pretokens` | Dumps a list of all the tokens in a the main source file before the preprocessing phase begins.         |
| `-dump-tokens`    | Dumps a list of all the tokens in a program after the preprocessing phase has completed.                |
| `-dump-debug`     | Dumps assorted debugging information produced during parsing to a file.                                 |
| `-no-time`        | Exclude the current time from the default timestamp included in the generated file.                     |
| `-timestamp`      | Replace the default timestamp with a custom timestamp provided after this argument.                     |

```
glulx-assemble -dump_tokens basic.ga basic.ulx
```


## Building glulx-assemble

glulx-assemble is written in standard C99; it should be possible to compile it with any compliant compiler. It is packaged using a simple makefile for building. There are no external dependencies.


## Source Files

Source files are text files with the ".ga" extension. The assembler expects them to be encoded using UTF-8. Some sample source files can be found in the demos directory; [basic.ga] provides a "hello world" style example. As a special case, the filename can be given as "-" (without the quotes) to read the source file from stdin.

A source file is a sequence of one-line statements that the assembler uses to create a glulx program file. Each line can begin with a label consisting of an identifier followed by a colon (`the_label_name:`). This is followed by the statement for that line.  A source line may also contain a comment; comments begin with a semicolon (`;`) and continue until the end of the line. All of these elements are optional and a source-line may contain all, some, or none of them. An example of a source line containing all three elements is included below:

```
start_loop: aloadb a_string #0 #1   ; load next character from string
```

There are two kinds of statements: instructions and directives. Instructions make up the majority of most assembly programs and are translated directly into glulx VM instructions. Directives give an instruction to the assembler and may produce output that is written to the glulx file.



### Instructions

An instruction statement consists of an opcode mnemonic followed by zero or more operands. For an index of opcodes and their expected operands, check the [relevant part of the glulx spec]. There are several types of values that an operand can have, summed up in the table below:


|          Type          |    Sample    |                                  Description                                   |
|------------------------|--------------|--------------------------------------------------------------------------------|
| Integer literal        | `-42`        |                                                                                |
| Hexadecimal literal    | `$7F`        |                                                                                |
| Floating-point literal | `63.5`       |                                                                                |
| Label name             | `start_loop` |                                                                                |
| Local variable name    | `index`      | The name of a local variable declared in the last encountered function header. |
| Local variable index   | `#2`         | A local variable by position.                                                  |
| Named constant         | `MAX_LENGTH` | Created with the `.define` directive.                                          |

There is also a special mnemonic `opcode` which allows the use of custom opcodes not known to the assembler. The word `opcode` may be immediately followed by `rel` to indicate that the last operand should be treated as a relative value akin to the jump directives in glulx. Next is the opcode number (or constant defined with the opcode number) followed by all the operands as normal.

```
opcode 112 32             ; print a single space character
opcode rel $20 some_label ; jump to "some_label"

.define MY_STREAMCHAR 112
opcode MY_STREAMCHAR 32             ; print a single space character
```

### Directives

Directives provide instructions to the assembler. Many produce output in the output file, but not all do. All directives begin with a dot to indicate that they are not assembler mnemonics.

Two directives are required in any valid glulx-assemble program: `.function` declaring a function named `start` to run when the program starts and `.end_header` to mark where the header data ends.

#### General

**.define**: Defines a symbol with a specified constant value. The value must be a numeric literal or a previously defined constant value.

A few constants are automatically defined. These are *_RAMSTART*, *_EXTSTART*, and *_ENDMEM* which have the same value as header fields of the same name.

```
.define MAX_LENGTH 512
```

**.end_header**: Marks the end of the header data. All information contained in the header will be read-only for the program. The length of the header will be padded with zero bytes to reach a 256 byte boundary.

```
.end_header
```

**.extra_memory**: Tells the interpreter to add this much zero-initialized memory to the end of the loaded program. This space is not included in the program file itself and cannot be manipulated by the assembler. It must be a multiple of 256 bytes.

```
.extra_memory 512
```

**.function**: Adds a function header to the output file. The directive may be immediately followed by `stk` to specify that the arguments to this function should be passed on the stack rather than copied into the local variables (see the [glulx spec][glulx spec 6.2] for details). This is followed by the local variable specification. Locals may be specified by either a positive integer (which will create that many unnamed local variables to be accessed by index) or with a list of local variable names. These styles may not be combined, but if a function has no local variables the specification may be omitted entirely.


Every glulx program must create at least one function identified by the label `start`. This will be the entry point of the program. Other functions should generally be identified by appropriate labels as well, though it is not technically required.

```
.function stk 7
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

#### File inclusion

**.include**: Process another file as though its contents occurred at this point in the current file. glulx-assemble will look for included files relative to the current directory, not the directory the current source file is located in. No include guards are used; infinite include loops will freeze or crash the assembler. Included files cannot be read from stdin and the filename "-" is forbidden.

```
.include "glk.ga"
```

**.include_binary**: Includes the raw binary content of another file at the current position of the output file. See the `.include` directive above for general details about the include system.

```
.include "great_art.png"
```

#### Raw Data Directives

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

**.unicode**: Includes an unencoded unicode string at the current position. A glulx unicode string is a series of four-byte codepoints.

```
.unicode "こんにちは世界。\n"
```

**.zero**: Include one or more zero bytes in the output file.

```
.zero 54
```

[basic.ga]: ./demos/basic.ga "View source file"
[GGASM]: https://github.com/GrenDrake/ggasm "Visit GGASM repository on GitHub"
[glulx spec 6.2]: https://www.eblong.com/zarf/glulx/glulx-spec_1.html#s.6.2 "Read Glulx official specs on this topic"
[glulx]: https://www.eblong.com/zarf/glulx/ "Visit Glulx homepage"
[relevant part of the glulx spec]: https://www.eblong.com/zarf/glulx/glulx-spec_2.html "Read Glulx official specs on this topic"

<!-- EOF -->
