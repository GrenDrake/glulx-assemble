#include "assemble.h"

struct mnemonic codes[] = {
    { "nop",           0x00,  0 },
    { "quit",          0x120, 0 },
    { "glk",           0x130, 3 },
    { "getiosys",      0x148, 2 },
    { "setiosys",      0x149, 2 },
    { "gestalt",       0x100, 3 },

    { "debugtrap",     0x101, 1 },
    { "getmemsize",    0x102, 1 },
    { "setmemsize",    0x103, 2 },
    { "random",        0x110, 2 },
    { "setrandom",     0x111, 1 },
    { "verify",        0x121, 1 },

    { "restart",       0x122, 0 },
    { "save",          0x123, 2 },
    { "restore",       0x124, 2 },
    { "saveundo",      0x125, 1 },
    { "restoreundo",   0x126, 1 },
    { "protect",       0x127, 2 },
    { "getstringtbl",  0x140, 1 },
    { "setstringtbl",  0x141, 1 },

    { "linearsearch",  0x150, 8 },
    { "binarysearch",  0x151, 8 },
    { "linkedsearch",  0x152, 7 },
    { "mzero",         0x170, 2 },
    { "mcopy",         0x171, 3 },
    { "malloc",        0x178, 2 },
    { "mfree",         0x179, 1 },
    { "accelfunc",     0x180, 2 },
    { "accelparam",    0x181, 2 },

    // integer math
    { "add",           0x10,  3 },
    { "sub",           0x11,  3 },
    { "mul",           0x12,  3 },
    { "div",           0x13,  3 },
    { "mod",           0x14,  3 },
    { "neg",           0x15,  2 },

    // bitwise operations
    { "bitand",        0x18,  3 },
    { "bitor",         0x19,  3 },
    { "bitxor",        0x1A,  3 },
    { "bitnot",        0x1B,  3 },
    { "shiftl",        0x1C,  3 },
    { "sshiftr",       0x1D,  3 },
    { "ushiftr",       0x1E,  3 },

    // floating conversions
    { "numtof",        0x190, 2 },
    { "ftonumz",       0x191, 2 },
    { "ftonumn",       0x192, 2 },

    // floating point math
    { "ceil",          0x198, 2 },
    { "floor",         0x199, 2 },
    { "fadd",          0x1A0, 3 },
    { "fsub",          0x1A1, 3 },
    { "fmul",          0x1A2, 3 },
    { "fdiv",          0x1A3, 3 },
    { "fmod",          0x1A4, 4 },
    { "sqrt",          0x1A8, 2 },
    { "exp",           0x1A9, 2 },
    { "log",           0x1AA, 2 },
    { "pow",           0x1AB, 3 },
    { "sin",           0x1B0, 2 },
    { "cos",           0x1B1, 2 },
    { "tan",           0x1B2, 2 },
    { "asin",          0x1B3, 2 },
    { "acos",          0x1B4, 2 },
    { "atan",          0x1B5, 2 },
    { "atan2",         0x1B6, 3 },

    // floating point branching
    { "jfeq",          0x1C0, 4, 1 },
    { "jfne",          0x1C1, 4, 1 },
    { "jflt",          0x1C2, 3, 1 },
    { "jfle",          0x1C3, 3, 1 },
    { "jfgt",          0x1C4, 3, 1 },
    { "jfge",          0x1C5, 3, 1 },
    { "jisnan",        0x1C8, 2, 1 },
    { "jisinf",        0x1C9, 2, 1 },

    // jumps
    { "jump",          0x20,  1, 1 },
    { "jz",            0x22,  2, 1 },
    { "jnz",           0x23,  2, 1 },
    { "jeq",           0x24,  3, 1 },
    { "jne",           0x25,  3, 1 },
    { "jlt",           0x26,  3, 1 },
    { "jge",           0x27,  3, 1 },
    { "jgt",           0x28,  3, 1 },
    { "jle",           0x29,  3, 1 },
    { "jltu",          0x2A,  3, 1 },
    { "jgeu",          0x2B,  3, 1 },
    { "jgtu",          0x2C,  3, 1 },
    { "jleu",          0x2D,  3, 1 },
    { "jumpabs",       0x2D,  1 },

    // function calls
    { "call",          0x30,  3 },
    { "return",        0x31,  1 },
    { "catch",         0x32,  2 },
    { "throw",         0x33,  2 },
    { "tailcall",      0x30,  2 },
    { "callf",         0x160, 2 },
    { "callfi",        0x161, 3 },
    { "callfii",       0x162, 4 },
    { "callfiii",      0x163, 5 },

    // moving data
    { "copy",          0x40,  2 },
    { "copys",         0x41,  2 },
    { "copyb",         0x42,  2 },
    { "sexs",          0x44,  2 },
    { "sexb",          0x45,  2 },
    { "aload",         0x48,  3 },
    { "aloads",        0x49,  3 },
    { "aloadb",        0x4A,  3 },
    { "aloadbit",      0x4B,  3 },
    { "astore",        0x4C,  3 },
    { "astores",       0x4D,  3 },
    { "astoreb",       0x4E,  3 },
    { "astorebit",     0x4F,  3 },

    // output operations
    { "streamchar",    0x70,  1 },
    { "streamnum",     0x71,  1 },
    { "streamstr",     0x72,  1 },
    { "streamunichar", 0x73,  1 },

    // stack operations
    { "stkcount",      0x50,  1 },
    { "stkpeek",       0x51,  2 },
    { "stkswap",       0x52,  0 },
    { "stkroll",       0x53,  2 },
    { "stkcopy",       0x54,  1 },

    { 0,               0,     0 }
};