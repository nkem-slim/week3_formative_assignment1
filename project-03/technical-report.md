# Project 3: CPython Data Analysis

## Implementation and Build

`python_baseline.py` and `data_analysis.c` implement mean, population variance,
minimum, maximum, outlier counting, and min-max normalization. Welford's
algorithm calculates mean and variance. An outlier is strictly more than the
configured number of standard deviations from the mean; values exactly on the
boundary are excluded. Constant datasets normalize to zeros. Empty datasets,
booleans, nonnumeric values, nonfinite values, and invalid thresholds are
rejected. Calculations use double-precision floating-point arithmetic, with
errors raised for unsupported numeric ranges.

From `project-03`, build and use the extension with:

```bash
python3 setup.py build_ext --inplace
python3 app.py python 1000 1.5
python3 app.py c 1000 1.5
```

The build requires a C compiler, setuptools, and the matching Python development
headers. `setup.py` compiles and links the C source into a shared object in this
directory. The recorded build produced
`data_analysis.cpython-314-x86_64-linux-gnu.so`, an extension for CPython 3.14 on
this platform. Python imports it as `data_analysis`; it is not a standalone
executable. Other Python versions or platforms generally require rebuilding.

## CPython Integration and Memory Management

On import, Python calls `PyInit_data_analysis`, which creates the module and
registers `analyze` through a method table. `PyArg_ParseTupleAndKeywords` accepts
the dataset and optional threshold. `PySequence_Fast` provides access to input
items, which are validated and converted into an allocated C double buffer.
The compiled loops perform the calculations, and `Py_BuildValue` returns a
dictionary with the same fields as the Python implementation, including a
Python list of normalized values.

The extension checks allocation and conversion failures, releases its C buffer
with `PyMem_Free`, and decrements owned Python references. References to input
items are borrowed. Newly created normalized floats are transferred to their
list, while the returned dictionary retains the list. The implementation keeps
the GIL throughout the call; it does not provide parallel execution of these
calculations across Python threads.

## Correctness

`python3 test_analysis.py` passed all 10 test methods in the saved run
(`correctness-results.txt`). Tests cover independently known results, small and
constant datasets, strict outlier boundaries, invalid and empty inputs, keyword
arguments, unchanged inputs, and equivalence at 1,000, 100,000, and 1,000,000
values. Floating-point comparisons use relative and absolute tolerances of
`1e-12`; outlier counts are compared exactly. These tests provide evidence for
the covered cases, rather than a proof for every possible numerical input.

## Benchmark Method and Results

The recorded environment was Python 3.14.4 on
Linux-7.0.0-38-generic-x86_64-with-glibc2.43. Processor identification was not
reported. Each size used the same seeded dataset for both implementations and
a threshold of 1.5 standard deviations. After one warm-up per implementation,
five measurements were taken with alternating execution order.
`time.perf_counter()` measured complete analysis calls: input validation,
calculations, and result allocation were included; imports, dataset generation,
and result destruction were excluded. Raw samples and environment information
are retained in `benchmark-results.txt`.

| Dataset size | Python average (seconds) | C average (seconds) | Speedup |
| --- | --- | --- | --- |
| 1,000 | 0.00058754 | 0.00002718 | 21.62x |
| 100,000 | 0.05146817 | 0.00362100 | 14.21x |
| 1,000,000 | 0.52180596 | 0.03968786 | 13.15x |

Speedup is Python average time divided by C average time. The extension is
faster at every measured size. From 100,000 to 1,000,000 values, Python time
increases about 10.14 times and C time about 10.96 times, broadly consistent
with linear work. Both implementations use O(n) time and O(n) additional space,
including converted inputs and normalized output.

The relative speedup decreases as size grows. This is compatible with increasing
memory and Python-object allocation costs in the extension, but the benchmark
does not isolate those causes. The 1,000-value calls are very short, and their
samples vary substantially, making that ratio more sensitive to timing noise.
Compiled loops avoid much of Python's per-iteration interpreter overhead, but
conversion, validation, and construction of Python results remain necessary.
Only the combined analysis operation was timed, so these results do not establish
separate speedups for mean, variance, extrema, outlier counting, or normalization.

## Trade-offs and Limitations

C adds build requirements, interpreter/platform compatibility constraints, and
manual reference and memory management. Incorrect C code can crash Python,
whereas ordinary Python errors usually raise exceptions. Input conversion and
Python result creation can reduce the benefit of native arithmetic. This
implementation accepts Python integers and floats, rather than all third-party
numeric types, and conversion to doubles can lose precision for large integers.
Extreme finite inputs can overflow intermediate calculations and are rejected.

The measurements describe one environment and five repetitions per size;
system load, allocation behavior, compiler settings, and hardware can affect
results. No separate operation was observed to slow down because operations
were measured together. Repeating the benchmark or profiling individual phases
would be necessary to attribute costs more precisely.
