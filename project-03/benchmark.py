import platform
import statistics
import sys
import time

import data_analysis
from python_baseline import analyze, generate_dataset


def measure(implementation, values, threshold):
    start = time.perf_counter()
    result = implementation(values, threshold)
    elapsed = time.perf_counter() - start
    # Keep result allocation inside the timing and its destruction outside.
    del result
    return elapsed


def main():
    repeats = 5
    threshold = 1.5
    print(f"Python: {sys.version.split()[0]}")
    print(f"Platform: {platform.platform()}")
    print(f"Processor: {platform.processor() or 'not reported'}")
    print(f"Repeated measurements per implementation: {repeats}")
    print(f"Outlier threshold: {threshold} standard deviations")
    print("Dataset generation and module imports excluded from timings.")
    print("Validation, calculations, and result allocation included.")
    print("One warm-up per implementation; measurement order alternates.")
    print()
    print("| Values | Python average (s) | C average (s) | Speedup |")
    print("| --- | --- | --- | --- |")

    for size in (1000, 100000, 1000000):
        values = generate_dataset(size)
        analyze(values, threshold)
        data_analysis.analyze(values, threshold)
        timings = {"python": [], "c": []}
        implementations = [("python", analyze), ("c", data_analysis.analyze)]

        for repeat in range(repeats):
            order = implementations if repeat % 2 == 0 else implementations[::-1]
            for name, implementation in order:
                timings[name].append(measure(implementation, values, threshold))

        python_average = statistics.mean(timings["python"])
        c_average = statistics.mean(timings["c"])
        speedup = python_average / c_average
        print(f"| {size:,} | {python_average:.8f} | {c_average:.8f} | {speedup:.2f}x |",
              flush=True)
        # Raw samples support the reported averages and reveal variability.
        print(f"Python samples (s): {', '.join(f'{value:.8f}' for value in timings['python'])}")
        print(f"C samples (s): {', '.join(f'{value:.8f}' for value in timings['c'])}")
        print()

    return 0


if __name__ == "__main__":
    sys.exit(main())
