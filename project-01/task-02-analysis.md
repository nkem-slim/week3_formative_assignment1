# Task 2: ELF structure

These findings describe the current binaries. Recompiling with different source,
compiler versions, or options can change addresses and sizes. Neither target was
executed, and `ldd` was not used.

## Compilation and inspection commands

Run from the repository root. Both binaries were built with GCC defaults and no
explicit compiler options other than `-o`, which selects the output filename:

```bash
gcc project-01/data_sync.c -o project-01/data_sync
gcc project-01/data_sync_v2.c -o project-01/data_sync_v2
```

The following commands capture the evidence without executing the targets:

```bash
file project-01/data_sync project-01/data_sync_v2 > project-01/task-02-file.txt
readelf -h project-01/data_sync > project-01/data_sync-header.txt
readelf -h project-01/data_sync_v2 > project-01/data_sync_v2-header.txt
readelf -SW project-01/data_sync > project-01/data_sync-sections.txt
readelf -SW project-01/data_sync_v2 > project-01/data_sync_v2-sections.txt
readelf -lW project-01/data_sync > project-01/data_sync-segments.txt
readelf -lW project-01/data_sync_v2 > project-01/data_sync_v2-segments.txt
```

`file` identifies the binary format and basic properties. `readelf -h` displays
the ELF header, `-S` displays section headers, and `-l` displays program headers
and the section-to-segment mapping. `-W` keeps each entry on a wide line.
The `>` operator saves output to a file, replacing its previous contents.

## ELF headers

Evidence: `task-02-file.txt`, `data_sync-header.txt`, and
`data_sync_v2-header.txt`.

| Property | data_sync | data_sync_v2 |
| --- | --- | --- |
| Class | ELF64 (64-bit) | ELF64 (64-bit) |
| Byte order | Little endian | Little endian |
| Machine | AMD x86-64 | AMD x86-64 |
| Type | DYN, position-independent executable (PIE) | DYN, position-independent executable (PIE) |
| Entry point | `0x1160` | `0x11e0` |
| Program headers | 14 | 14 |
| Section headers | 31 | 31 |
| Interpreter | `/lib64/ld-linux-x86-64.so.2` | `/lib64/ld-linux-x86-64.so.2` |

ELF64 identifies the 64-bit ELF format. Little endian means the least significant
byte of a multi-byte value is stored first. The machine field identifies the
x86-64 instruction set; the AMD name does not require an AMD-branded processor.

Both files are identified as PIE executables. Although their ELF type is DYN,
this does not make them ordinary shared libraries: `file` and `readelf` explicitly
identify them as executables. PIE permits loading at a varying base address.
The entry values are ELF virtual addresses, not fixed absolute runtime addresses;
the load bias must be added for the actual process address. They identify the
executable's startup entry, which must not be assumed to be `main`. With an
interpreter present, the dynamic loader runs before handing control to that entry.

## Program headers and segments

Evidence: `data_sync-segments.txt` and `data_sync_v2-segments.txt`.
Both binaries contain the following program header types:

| Type | Purpose and observed evidence |
| --- | --- |
| PHDR | Describes the program header table itself. |
| INTERP | Names the dynamic loader shown above. |
| LOAD (four entries) | Maps file contents into memory: read-only metadata, readable/executable code, read-only constants, and readable/writable data. |
| DYNAMIC | Locates dynamic-linking metadata. |
| NOTE (three entries) | Describes notes including the build ID, GNU properties, and ABI information. |
| GNU_PROPERTY | Identifies GNU processor/property metadata. |
| GNU_EH_FRAME | Locates the frame-unwinding header. |
| GNU_STACK | Requests a readable/writable stack without execute permission (`RW`, no `E`). |
| GNU_RELRO | Identifies a region intended to become read-only after relocations. |

The writable LOAD segment in A has file size `0x298` and memory size `0x2b8`;
in B these are `0x2b8` and `0x2d8`. The larger memory size accommodates zero-filled
storage and alignment, including `.bss`. These headers describe requested loading
properties; static inspection alone does not demonstrate runtime enforcement.

## Important sections

Evidence: `data_sync-sections.txt` and `data_sync_v2-sections.txt`.
Every section listed below is present in both binaries. Sizes are bytes.

| Section | Purpose | A size | B size |
| --- | --- | --- | --- |
| `.text` | Compiled executable instructions, including startup and program functions | `0x3c4` (964) | `0x69d` (1693) |
| `.rodata` | Read-only constants, such as message strings | `0x15e` (350) | `0x25e` (606) |
| `.data` | Initialized writable data, including compiler/runtime data | `0x10` (16) | `0x10` (16) |
| `.bss` | Zero-initialized storage; NOBITS means no corresponding data bytes in the file | `0x10` (16) | `0x10` (16) |
| `.symtab` | Regular symbol table used by analysis/linking tools | `0x468` (1128) | `0x4f8` (1272) |
| `.strtab` | Names referenced by the regular symbol table | `0x2a4` (676) | `0x316` (790) |
| `.dynsym` | Symbols used for dynamic linking | `0x180` (384) | `0x1e0` (480) |
| `.dynamic` | Dynamic-linker tags and associated values | `0x1f0` (496) | `0x1f0` (496) |

Both also contain `.dynstr`, which holds dynamic-linking strings, and `.shstrtab`,
which holds section names. The larger code, constant, and symbol sections in B
are consistent with its added metadata comparison and path-building code. Section
sizes alone cannot establish the program's behavior or whether it is malicious.

## Debugging information

Neither section listing contains `.debug_info`, `.debug_line`, or other
`.debug_*` sections. Thus these files contain no embedded DWARF debugging
information visible in the section listings. Both retain `.symtab` and `.strtab`,
consistent with `file` reporting `not stripped`. Retained symbol names are useful
for analysis but do not establish the presence of source line debugging data.

## Sections versus segments

Sections organize code, data, symbols, and other information for linking and
analysis. Segments describe how the loader maps regions into memory and the
permissions requested for those mappings. A segment can include multiple sections:
the executable LOAD segment contains `.init`, the PLT sections, `.text`, and
`.fini`. The regular symbol and string tables are outside the mapped segments,
because the program does not need them for normal execution. The mapping shown
by `readelf -lW` provides direct evidence of this distinction.
