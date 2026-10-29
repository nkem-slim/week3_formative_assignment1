import argparse
import sys

from python_baseline import analyze, generate_dataset


def main():
    parser = argparse.ArgumentParser(description="Run Python or C data analysis")
    parser.add_argument("implementation", choices=("python", "c"))
    parser.add_argument("size", type=int, nargs="?", default=1000)
    parser.add_argument("threshold", type=float, nargs="?", default=2.0)
    arguments = parser.parse_args()

    if arguments.implementation == "c":
        try:
            from data_analysis import analyze as analyze_values
        except ImportError:
            print("Build the extension first: python3 setup.py build_ext --inplace",
                  file=sys.stderr)
            return 1
    else:
        analyze_values = analyze

    try:
        values = generate_dataset(arguments.size)
        result = analyze_values(values, arguments.threshold)
    except (ValueError, TypeError, OverflowError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    print(f"Implementation: {arguments.implementation}")
    print(f"Dataset size: {arguments.size}")
    print(f"Threshold (standard deviations): {arguments.threshold}")
    for name in ("mean", "variance", "minimum", "maximum", "outlier_count"):
        print(f"{name}: {result[name]}")
    print(f"First five normalized values: {result['normalized'][:5]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
