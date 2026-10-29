import math
import random
import sys


def validate_values(values):
    """Accept a nonempty sequence of finite real numbers."""
    numbers = []
    for value in values:
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            raise TypeError("Dataset values must be integers or floats")
        number = float(value)
        if not math.isfinite(number):
            raise ValueError("Dataset values must be finite")
        numbers.append(number)

    if not numbers:
        raise ValueError("Dataset must not be empty")
    return numbers


def analyze(values, threshold=2.0):
    """Count outliers beyond threshold standard deviations from the mean."""
    numbers = validate_values(values)
    if isinstance(threshold, bool) or not isinstance(threshold, (int, float)):
        raise TypeError("Threshold must be an integer or float")
    threshold = float(threshold)
    if not math.isfinite(threshold) or threshold < 0:
        raise ValueError("Threshold must be finite and nonnegative")

    # Welford's algorithm calculates population variance in one pass.
    mean = 0.0
    squared_deviations = 0.0
    minimum = numbers[0]
    maximum = numbers[0]
    for count, number in enumerate(numbers, start=1):
        delta = number - mean
        mean += delta / count
        squared_deviations += delta * (number - mean)
        if number < minimum:
            minimum = number
        if number > maximum:
            maximum = number

    variance = max(squared_deviations / len(numbers), 0.0)
    value_range = maximum - minimum
    if not all(math.isfinite(value) for value in (mean, variance, value_range)):
        raise OverflowError("Dataset exceeds the supported floating-point range")

    cutoff = threshold * math.sqrt(variance)
    outlier_count = 0
    normalized = []
    for number in numbers:
        if abs(number - mean) > cutoff:
            outlier_count += 1
        if value_range == 0:
            normalized.append(0.0)
        else:
            normalized.append((number - minimum) / value_range)

    return {
        "mean": mean,
        "variance": variance,
        "minimum": minimum,
        "maximum": maximum,
        "outlier_count": outlier_count,
        "normalized": normalized,
    }


def generate_dataset(size, seed=42):
    if size <= 0:
        raise ValueError("Dataset size must be positive")
    generator = random.Random(seed)
    return [generator.uniform(-100.0, 100.0) for _ in range(size)]


def main():
    if len(sys.argv) > 3:
        print("Usage: python3 python_baseline.py [SIZE] [THRESHOLD]", file=sys.stderr)
        return 1

    try:
        size = int(sys.argv[1]) if len(sys.argv) > 1 else 1000
        threshold = float(sys.argv[2]) if len(sys.argv) > 2 else 2.0
        result = analyze(generate_dataset(size), threshold)
    except (ValueError, TypeError, OverflowError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1

    print(f"Dataset size: {size}")
    print(f"Threshold (standard deviations): {threshold}")
    for name in ("mean", "variance", "minimum", "maximum", "outlier_count"):
        print(f"{name}: {result[name]}")
    print(f"First five normalized values: {result['normalized'][:5]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
