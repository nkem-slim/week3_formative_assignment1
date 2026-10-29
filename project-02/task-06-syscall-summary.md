# Task 6: System Call Summary

Evidence: `backup_success_trace.txt` and `backup_failed_trace.txt`.
The successful run used `source.text`; the failed run used
`missing_source.txt`. Process IDs identify these recorded runs only.

| System call | Purpose | Relevant arguments | Observed return value or error | Role in the program |
| --- | --- | --- | --- | --- |
| `execve` | Loads a program image | `"./backup_tool"`, arguments containing `"source.text"` or `"missing_source.txt"` | `0` in both traces | Starts the traced executable; the child does not perform another `execve` |
| `mkdir` | Creates a directory | `"backup", 0755` | `-1 EEXIST` in both traces | Attempts directory creation; an existing directory is accepted after validation |
| `newfstatat` | Retrieves path metadata | `AT_FDCWD, "backup", ..., 0` | `0`, with `st_mode=S_IFDIR\|0755` | Confirms that the existing backup path is a directory |
| `openat` | Opens or creates a file | Source: `"source.text", O_RDONLY`; destination: `"backup/source.txt", O_WRONLY\|O_CREAT\|O_NONBLOCK, 0644`; log: `"backup.log", O_WRONLY\|O_CREAT\|O_TRUNC, 0644` | Success: source descriptor `4`, destination `5`, log `3`; failure: source open returns `-1 ENOENT` | Opens backup resources; failed source open identifies the controlled failure |
| `fstat` | Retrieves metadata through an open descriptor | Source descriptor `4` and destination descriptor `5` in the successful child | `0`; both are regular files with recorded size `53` before truncation | Supports regular-file and same-file checks; other occurrences inspect runtime files or output streams |
| `ftruncate` | Changes file length | Destination descriptor `5`, length `0` | `0` in success; absent in failed child | Clears the destination before copying; failure occurs before this operation |
| `read` | Reads bytes from a descriptor | Source descriptor `4`, buffer capacity `4096` | `53`, then `0` in success | Reads source content, then detects end-of-file; initialization also reads libc separately |
| `write` | Writes bytes to a descriptor | Destination descriptor `5`, count `53`; log descriptor `3`; standard output/error descriptors | Destination write returns `53`; child failure-log write returns `21`; parent failure-log write returns `27` | Copies data and records progress or errors; the destination write is absent in failure |
| `close` | Releases a descriptor | Success: source `4`, destination `5`, and each process's log descriptor `3` | `0` for these observed closes | Releases backup resources; closing the child's inherited log descriptor does not close the parent's copy |
| `clone` | Creates the child process | `CLONE_CHILD_CLEARTID\|CLONE_CHILD_SETTID\|SIGCHLD`, null child stack | Parent receives `1376230` in success and `1391126` in failure | Linux/glibc implementation of the program's `fork()` call in these traces |
| `wait4` | Waits for and collects child status | Child PID, status pointer, options `0`, resource pointer `NULL` | Returns `1376230` with exit status `0` in success; `1391126` with exit status `1` in failure | Implements `waitpid()` here; synchronizes parent with child and communicates the result |
| `exit_group` | Terminates the calling process and its threads | Status `0` in success, `1` in failure | No normal return; strace shows `= ?` followed by an exit annotation | Terminates child and parent with their recorded outcomes |
| `access` | Checks path accessibility | `"/etc/ld.so.preload", R_OK` | `-1 ENOENT` in both traces | Dynamic-loader initialization; unrelated to locating the backup source |
| `dup` | Duplicates a descriptor | Standard error descriptor `2` | Returns descriptor `4` in the failed child | Appears in library error reporting after the source open fails |
| `fcntl` | Inspects descriptor flags | `4, F_GETFL` | `0x2`, reported as `O_RDWR`, in failure | Supports the library error-reporting path; does not open the backup source |

## Reading the Evidence

`AT_FDCWD` means that a relative path is resolved from the process's current
working directory. Descriptors are process-local handles and may be reused:
descriptor `4` identifies the source in the successful child, but a duplicate
of standard error in the failed child. A nonnegative descriptor from `openat`
indicates success; `0` commonly indicates success for calls such as `close`;
`read` and `write` return byte counts. `read` returning `0` means end-of-file.

The parent PIDs are `1376229` for success and `1391122` for failure. The
`<unfinished ...>` and `<... resumed>` entries represent interleaved tracing
of concurrent processes, not additional system calls or errors. The exit
status decoded by `wait4` is distinct from its return value, which is the
collected child's PID. `SIGCHLD` is a delivered signal, not a system call.

Both runs encountered an already existing backup directory, so these traces
demonstrate the existing-directory branch rather than successful creation of
a new directory. In contrast, the failed source open is fatal to the backup.
The loader's missing optional preload file is not fatal in either recorded run.
