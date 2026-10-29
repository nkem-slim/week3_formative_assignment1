Both binaries are dynamically linked. Each declares `libc.so.6` through a NEEDED entry, and both specify a dynamic interpreter. V2 imports additional functions while retaining the same direct library dependency. Imported symbols identify external references; they do not prove that the corresponding operations execute at runtime. Dependencies were inspected statically without executing either target or using `ldd`.

## Symbols and Dependencies

| Term | Meaning | Recorded example |
| --- | --- | --- |
| Undefined symbol | A referenced symbol whose definition is not supplied by this binary | `U printf` in `nm` output |
| Dynamically imported symbol | An external symbol referenced through the dynamic-linking machinery | Undefined `printf` in `nm -D` and `objdump -T` |
| Shared-library dependency | A library declared as required by the binary, rather than an individual function | `NEEDED libc.so.6` in `readelf -d` and `objdump -p` |

An undefined symbol alone does not identify which library will supply it.
Undefined weak references may remain unresolved without preventing loading.
The GLIBC suffixes identify symbol versions, not separate library dependencies.

## Why Symbols May Be Missing

Stripping can remove the regular symbol table, so ordinary `nm` may report no
symbols while `nm -D` still shows dynamic symbols needed for loading. Compiler
optimization can inline or eliminate functions, and linker settings can discard
unused code. Functions such as `main` can remain in the regular table without
being exported in the dynamic table. Therefore, absence from one symbol listing
does not by itself prove that related functionality is absent.

## Additional Dynamic Entries

Both recorded dynamic sections contain `BIND_NOW` and `NOW PIE`, requesting
immediate symbol resolution and identifying PIE. V2's `PLTRELSZ` is 312 bytes
versus 216 in A, consistent with additional PLT relocation entries. The `DEBUG`
tag is for dynamic-loader debugging; it does not establish embedded source-level
debugging information. Declared direct dependencies do not enumerate every
library that could be loaded indirectly at runtime.
