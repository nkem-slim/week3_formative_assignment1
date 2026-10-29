# Forensic Assessment: data_sync and data_sync_v2

## Purpose and Methodology

The investigation compared two controlled-source file synchronization simulations
to assess their structure and likely functionality. Both were compiled with
`gcc SOURCE -o OUTPUT`, using GCC defaults. Static inspection used `file`,
`readelf`, `nm`, and `objdump`. Neither target was executed, and `ldd` was not
used. Complete command outputs are submitted separately in this directory.

## Key Findings

Both files are ELF64, little-endian, x86-64 position-independent executables.
Their ELF entry points are `0x1160` for `data_sync` and `0x11e0` for
`data_sync_v2`, matching their `_start` symbols. Disassembly shows startup code
referencing `main` through `__libc_start_main`; `main` is at `0x13fc` and
`0x1742`, respectively. These ELF addresses are relative to the PIE load bias,
not fixed absolute runtime addresses.

Both contain `.text`, `.rodata`, `.data`, `.bss`, `.symtab`, `.strtab`,
`.dynsym`, and `.dynamic`. Each has 14 program headers, including four LOAD
entries separating metadata, executable code, constants, and writable data.
Regular symbol tables remain, but neither section listing contains embedded
`.debug_*` information. Both declare `libc.so.6` as their sole direct shared-library
dependency and specify `/lib64/ld-linux-x86-64.so.2` as interpreter, supporting
the conclusion that they are dynamically linked.

## Comparative Assessment

Both reference directory functions and `strcmp`, consistent with directory
scanning and entry-name comparison. V2 additionally defines `build_path` and
`compare_metadata`. Its disassembly contains a `snprintf` call at `0x138c`
and `stat` calls at `0x141a` and `0x1451`, followed by error checks, file-type
checks, and metadata comparisons. This supports an extension that compares
file size and modification time rather than names alone.

V2's `.text` grows from 964 to 1,693 bytes and `.rodata` from 350 to 606 bytes,
consistent with additional logic and messages. Its stack-canary checks and
`__stack_chk_fail` reference show compiler-generated protection. The added
imports do not introduce another declared direct library dependency.

## Conclusion and Limitations

The observed structure and call sites support related directory-comparison
utilities, with V2 adding metadata checks. They do not establish malicious
intent or prove that any particular operation occurs at runtime. Actual
execution paths, input-dependent failures, and successful synchronization
cannot be confirmed through this inspection. Matching metadata does not prove
matching contents. Addresses and sizes describe these builds and may change
with source changes or different compiler settings.
