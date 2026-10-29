# Task 5: Binary Comparison

The comparison below uses the static inspection outputs collected in Tasks 2–4.
Neither binary was executed, and `ldd` was not used.

| Property | `data_sync` | `data_sync_v2` |
| --- | --- | --- |
| ELF class and architecture | ELF64, little endian, x86-64 | ELF64, little endian, x86-64 |
| ELF file type | DYN, position-independent executable (PIE) | DYN, position-independent executable (PIE) |
| Entry-point address | `0x1160`, matching `_start` | `0x11e0`, matching `_start` |
| Presence of `main()` symbol | Present at `0x13fc` | Present at `0x1742` |
| Important sections | `.text`, `.rodata`, `.data`, `.bss`, `.symtab`, `.strtab`, `.dynsym`, `.dynamic` | Same sections present |
| Dynamic dependencies | `libc.so.6` | `libc.so.6` |
| Relevant imported functions | `opendir`, `readdir`, `rewinddir`, `closedir`, `strcmp`, `printf`, `fprintf`, `perror` | Same, plus `stat`, `snprintf`, `fwrite`, `__stack_chk_fail` |
| Debugging symbols | Regular symbols retained; no embedded `.debug_*` sections observed | Regular symbols retained; no embedded `.debug_*` sections observed |
| Significant differences | Directory scanning and name-comparison call sites | Adds `build_path` and `compare_metadata`, with path-formatting and metadata-inspection call sites |

## Evidence

- `file` and `readelf -h` identify both binaries as ELF64, little endian,
  x86-64 PIE executables and provide their entry-point addresses.
- `readelf -S` identifies the sections and their sizes. Both retain `.symtab`
  and `.strtab`, consistent with the `not stripped` result from `file`.
  Neither section listing shows embedded `.debug_*` sections.
- `nm -n`, `readelf -s`, and `objdump -t` identify `_start`, `main`, and
  the application functions. The `_start` addresses match the ELF entries;
  these are executable startup addresses, not the addresses of `main`.
- `nm -D` and `objdump -T` identify dynamically referenced symbols.
  `readelf -d` and `objdump -p` identify `libc.so.6` as the sole declared
  direct shared-library dependency in each binary.
- V2's disassembly contains a call to `snprintf` at `0x138c`, calls to `stat`
  at `0x141a` and `0x1451`, and a call to `compare_metadata` at `0x170c`.
  These are compiled call sites, not observations of runtime execution.

## Interpretation of the Differences

Both binaries have the same architecture, PIE format, and declared direct
shared-library dependency. V2 adds two application functions and references
additional library functions. Its calls to `snprintf` and `stat`, followed by
error checks and metadata comparisons, support the interpretation that it
extends name comparison with path construction and file metadata checks.
Additional imported functions do not necessarily require additional libraries.

V2's `.text` section grows from `0x3c4` (964) to `0x69d` (1,693) bytes,
and `.rodata` grows from `0x15e` (350) to `0x25e` (606) bytes. These increases
are consistent with the additional instructions and messages. Changes in the
compiled layout account for different function and entry-point addresses;
the address differences alone do not indicate different intent. Because these
are PIE executables, the recorded ELF addresses also require a load bias to
obtain actual runtime addresses.

Both programs were compiled using GCC defaults with only `-o` explicitly
specified. Neither compilation explicitly requested optimization, debugging
information, static linking, or stripping. Compiler defaults can still add
protections or transform library calls. V2's disassembly contains stack-canary
checks and calls to `__stack_chk_fail`. Its `build_path` error-output branch
calls `fwrite`, although the source expresses that output using `fprintf`.
Thus imported names do not always correspond directly to source-level calls.

Different optimization settings could inline or remove functions and change
code size. Static linking could include library implementations in the binary
instead of resolving them through shared libraries. Stripping could remove
regular function names while retaining symbols needed for dynamic linking.
These are possible effects of alternative build options, not changes observed
between the two recorded compilation commands.

The evidence supports a functional extension in V2, but does not establish
which paths execute for particular inputs or prove malicious intent. Matching
file size and modification time would also not establish identical contents.
