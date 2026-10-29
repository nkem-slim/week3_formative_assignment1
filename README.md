# Week 3 Formative Assignment 1

Four GNU/Linux projects exploring ELF binary analysis, system calls, CPython
extensions, and Unix signal handling. Each project directory contains its source
code, recorded evidence, and technical explanations.

## Requirements

- GNU/Linux with GCC and GNU binutils (`readelf`, `nm`, and `objdump`).
- `file` and `strace`.
- Python 3, setuptools, and matching Python development headers for Project 3.

The submitted Python extension was built for CPython 3.14 on Linux x86-64.
Rebuild it when using a different interpreter or platform. Commands below start
from the repository root unless a project directory is explicitly selected.

## Project 1: ELF Binary Forensics

`data_sync.c` compares directory-entry names. `data_sync_v2.c` adds path
construction and regular-file size and modification-time comparison.
These are synchronization simulations; they do not copy files.

Compile both controlled-source programs:

```bash
gcc project-01/data_sync.c -o project-01/data_sync
gcc project-01/data_sync_v2.c -o project-01/data_sync_v2
```

Inspect the binaries statically. Do not execute either target or use `ldd`:

```bash
file project-01/data_sync project-01/data_sync_v2
readelf -h project-01/data_sync
readelf -S project-01/data_sync
readelf -l project-01/data_sync
readelf -s project-01/data_sync
readelf -d project-01/data_sync
nm -n project-01/data_sync
nm -D project-01/data_sync
objdump -t project-01/data_sync
objdump -T project-01/data_sync
objdump -d project-01/data_sync
objdump -p project-01/data_sync
```

Repeat the inspection commands for `project-01/data_sync_v2`. The saved `.txt`
files contain the recorded command outputs. Rebuilding can change the addresses
and sizes, so comparisons must use evidence from the corresponding build.

Written deliverables:

- [ELF structure analysis](project-01/task-02-analysis.md)
- [Symbol interpretation](project-01/task-03-interpretation.md)
- [Linking explanation](project-01/task-04-explanation.md)
- [Comparison table and assessment](project-01/task-05-comparison.md)
- [Forensic report](project-01/task-06-report.md)

## Project 2: System Call Forensics

`backup_tool.c` creates or validates a backup directory, writes numbered log
entries, and uses a child process to copy the supplied source into
`backup/source.txt`. The parent waits for the child and propagates its result.
Run inside `project-02` so generated files stay in that directory.

```bash
cd project-02
gcc backup_tool.c -o backup_tool
echo "Backup test content" > source.text
strace -f -o backup_success_trace.txt ./backup_tool source.text
cmp source.text backup/source.txt
strace -f -o backup_failed_trace.txt ./backup_tool missing_source.txt
echo $?
cd ..
```

The failure experiment requires `missing_source.txt` to be absent. The expected
failure status is `1`; `cmp` produces no output when the copied contents match.
Each run replaces `backup.log`. Rerunning the commands replaces the saved traces;
their PIDs, contents, and observed calls may differ from the recorded analysis.

Deliverables:

- [Successful execution trace](project-02/backup_success_trace.txt)
- [Failed execution trace](project-02/backup_failed_trace.txt)
- [Observed system call table](project-02/task-06-syscall-summary.md)
- [Technical analysis](project-02/task-07-analysis.md)

## Project 3: CPython Data Analysis

The pure Python implementation and C extension calculate mean, population
variance, minimum, maximum, outlier count, and min-max normalized values.
The threshold specifies standard deviations from the mean; the outlier
comparison is strict. Constant datasets normalize to zeros.

```bash
cd project-03
python3 setup.py build_ext --inplace
python3 app.py python 1000 1.5
python3 app.py c 1000 1.5
python3 test_analysis.py
python3 benchmark.py
cd ..
```

Python imports the compiled `.so` module as `data_analysis`; it is not a
standalone executable. `test_analysis.py` checks correctness and equivalence,
including edge cases and all three required dataset sizes. The benchmark takes
five measurements per implementation at 1,000, 100,000, and 1,000,000 values.

Deliverables:

- `python_baseline.py`, `data_analysis.c`, and `setup.py`
- `app.py`, `test_analysis.py`, and `benchmark.py`
- `data_analysis.cpython-314-x86_64-linux-gnu.so`
- [Correctness results](project-03/correctness-results.txt)
- [Benchmark results and raw samples](project-03/benchmark-results.txt)
- [Technical report](project-03/technical-report.md)

The recorded combined-operation speedups were 21.62x, 14.21x, and 13.15x,
respectively. These are measurements from the recorded environment, not
guarantees for other machines or individual operations.

## Project 4: Signal-Driven Service Controller

The controller starts in NORMAL mode and reports every five seconds. Signal
handlers set flags; the main loop performs output, state changes, and shutdown.

```bash
cd project-04
gcc server_controller.c -o server_controller
./server_controller | tee signal-demo.log
```

In a second terminal, replace `PID` below with the number printed by the
controller. Send each command separately and wait for its response:

```bash
kill -SIGUSR1 PID
kill -SIGUSR2 PID
kill -SIGUSR1 PID
kill -SIGUSR2 PID
kill -SIGUSR1 PID
kill -SIGINT PID
```

SIGUSR1 reports mode and PID. Each SIGUSR2 toggles the mode, and SIGINT requests
graceful shutdown. Start a separate run for SIGTERM, using its new PID:

```bash
./server_controller | tee signal-term-demo.log
```

```bash
kill -SIGTERM PID
```

SIGTERM prints the emergency termination message and ends the service. The
existing logs show all four signals and transitions in both directions:

- [Status, maintenance, and graceful shutdown](project-04/signal-demo.log)
- [Emergency termination](project-04/signal-term-demo.log)
- [Mode transitions in both directions](project-04/signal-toggle-demo.log)
- [Technical explanation](project-04/technical-explanation.md)

## Submission Notes

Keep recorded evidence with its matching analysis. Generated executables,
temporary build directories, and test outputs may exist locally without being
tracked; the compiled CPython extension is included as a required deliverable.
Check the final formatted page count for Project 1's forensic report and
Project 4's technical explanation: each has a maximum of one page.
