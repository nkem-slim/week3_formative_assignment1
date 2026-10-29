# Backup System Call Analysis

## Method and Experiments

The program was compiled with `gcc backup_tool.c -o backup_tool` and traced
from `project-02` using `strace -f -o TRACE_FILE ./backup_tool SOURCE_FILE`.
The `-f` option includes child-process activity. Experiment A used the existing
`source.text`; Experiment B used the nonexistent `missing_source.txt`.
Evidence is saved in `backup_success_trace.txt` and `backup_failed_trace.txt`.
The observed calls are summarized in `task-06-syscall-summary.md`.

## File Operations (Task 3)

Both runs attempt `mkdir("backup", 0755)`, which returns `-1 EEXIST`.
The following `newfstatat` returns `0` and identifies a directory. The program
therefore handles an existing backup directory correctly. These traces do not
demonstrate the branch that creates a previously absent directory.

The parent opens `backup.log` with `O_WRONLY|O_CREAT|O_TRUNC` and receives
descriptor `3`. It writes numbered log entries; truncation means that each
run replaces the previous log. In the successful child, the following excerpts
show the source and destination being opened and the destination cleared:

```text
1376230 openat(AT_FDCWD, "source.text", O_RDONLY) = 4
1376230 openat(AT_FDCWD, "backup/source.txt", O_WRONLY|O_CREAT|O_NONBLOCK, 0644) = 5
1376230 ftruncate(5, 0)                 = 0
```

`AT_FDCWD` resolves these relative paths from the current directory. `openat`
creates process-local descriptors; the flags specify read-only source access
and writable destination access with creation permitted. The successful
`fstat` calls identify both opened files as regular files. The destination
already had a recorded size of 53 bytes before truncation, so this trace
demonstrates opening and replacing an existing destination rather than proving
creation of a new destination file.

The kernel transfers bytes through `read` and `write`:

```text
1376230 read(4, "I am testing using this backup t"..., 4096) = 53
1376230 write(5, "I am testing using this backup t"..., 53) = 53
1376230 read(4, "", 4096)               = 0
1376230 close(4)                        = 0
1376230 close(5)                        = 0
```

The child reads 53 bytes, writes all 53, reaches end-of-file, and releases both
descriptors. This supports a completed transfer in the recorded run, although
the abbreviated trace strings alone do not establish full content equality
or persistence after a power failure. The source code handles partial writes
and interrupted reads/writes, but neither condition appears in these traces.

## Process Creation and Synchronization (Task 4)

The initial `execve` loads the executable. The child later begins through
`fork()`, implemented here as `clone`, rather than another `execve`:

```text
1376229 clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x785c0a8daa10) = 1376230
1376230 exit_group(0)                   = ?
1376229 <... wait4 resumed>, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 1376230
```

Parent `1376229` creates child `1376230` and waits while the child copies.
The library implements `waitpid()` through `wait4` on this system. Its return
value is the child's PID; the separately decoded status shows successful exit.
The parent then writes its confirmation log entry and exits with status `0`.
Each process closes its own inherited log descriptor. Waiting ensures that
the parent's final entry follows the child's entry.

`<unfinished ...>` and `<... resumed>` show interleaved process activity,
not errors. `SIGCHLD` records notification of child termination, and
`exit_group` has no normal return value.

## Successful Versus Failed Execution (Task 5)

In Experiment B, parent `1391122` creates child `1391126`. The child's source
open is split across interleaved trace lines:

```text
1391126 openat(AT_FDCWD, "missing_source.txt", O_RDONLY <unfinished ...>
1391126 <... openat resumed>)           = -1 ENOENT (No such file or directory)
1391126 write(3, "2. Child copy failed\n", 21) = 21
1391126 exit_group(1)                   = ?
1391122 <... wait4 resumed>, [{WIFEXITED(s) && WEXITSTATUS(s) == 1}], 0, NULL) = 1391126
1391122 exit_group(1)                   = ?
```

The `ENOENT` result identifies the missing source as the cause, independently
of printed error messages. No destination open, truncation, or content copy
occurs in this child. The existing destination is therefore untouched by this
failure path. The child logs failure, closes its log descriptor, and exits with
status `1`. The parent collects that status, logs failure, closes its descriptor,
and also exits with `1`. This demonstrates appropriate failure propagation for
the tested missing-source case; permission and mid-copy failures were not tested.

## Runtime Activity and Additional Calls

The early opens of `/etc/ld.so.cache` and `libc.so.6`, along with `mmap`,
`mprotect`, and related calls, support dynamic loading and runtime setup.
Their reads do not copy the backup source. The loader's
`access("/etc/ld.so.preload", R_OK) = -1 ENOENT` appears in both experiments
and does not prevent startup. It must not be confused with the fatal missing
source in Experiment B.

The failure trace additionally contains `dup(2) = 4`, `fcntl(4, F_GETFL)`,
`fstat(4, ...)`, `write(4, ...)`, and `close(4)` during the library's `perror`
path. Descriptor `4` here refers to duplicated standard error, not the source
file used in the other run. Library buffering also accounts for output-stream
metadata checks. These additional calls support the runtime and error reporting;
the traces do not establish that they are unnecessary backup work.

## Reliability Improvement (Task 7)

The successful trace shows `ftruncate(5, 0)` before copying. A later read or write
failure could therefore destroy the previous complete backup and leave partial
output. A stronger implementation would write a temporary file in the backup
directory and replace the destination with `rename()` only after a successful
copy and close. For crash durability, it should also synchronize the temporary
file and directory appropriately. This is a proposed improvement, not behavior
observed in the submitted traces or implemented in the current source.
