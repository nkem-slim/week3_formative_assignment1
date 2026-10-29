import math
import unittest

import data_analysis
from python_baseline import analyze, generate_dataset


class AnalysisTests(unittest.TestCase):
    def assert_equivalent(self, values, threshold=2.0):
        expected = analyze(values, threshold)
        actual = data_analysis.analyze(values, threshold)
        self.assertEqual(actual.keys(), expected.keys())
        self.assertEqual(actual["outlier_count"], expected["outlier_count"])
        for name in ("mean", "variance", "minimum", "maximum"):
            self.assertTrue(math.isclose(actual[name], expected[name],
                                         rel_tol=1e-12, abs_tol=1e-12), name)
        self.assertEqual(len(actual["normalized"]), len(expected["normalized"]))
        for position, (left, right) in enumerate(zip(actual["normalized"],
                                                    expected["normalized"])):
            if not math.isclose(left, right, rel_tol=1e-12, abs_tol=1e-12):
                self.fail(f"Normalized value differs at index {position}")

    def test_known_results(self):
        for implementation in (analyze, data_analysis.analyze):
            with self.subTest(implementation=implementation.__module__):
                result = implementation([1, 2, 3], 1)
                self.assertEqual(result["mean"], 2.0)
                self.assertAlmostEqual(result["variance"], 2 / 3)
                self.assertEqual(result["minimum"], 1.0)
                self.assertEqual(result["maximum"], 3.0)
                self.assertEqual(result["outlier_count"], 2)
                self.assertEqual(result["normalized"], [0.0, 0.5, 1.0])

    def test_small_datasets(self):
        for values in ([5], [7, 7, 7], [-5, -1, 0, 3.5, 100], (1, 2.5, 4)):
            for threshold in (0, 1, 2):
                with self.subTest(values=values, threshold=threshold):
                    self.assert_equivalent(values, threshold)

    def test_constant_normalization(self):
        for implementation in (analyze, data_analysis.analyze):
            result = implementation([7, 7, 7])
            self.assertEqual(result["variance"], 0.0)
            self.assertEqual(result["outlier_count"], 0)
            self.assertEqual(result["normalized"], [0.0, 0.0, 0.0])

    def test_strict_outlier_boundary(self):
        for implementation in (analyze, data_analysis.analyze):
            self.assertEqual(implementation([-1, 1], 1)["outlier_count"], 0)

    def test_required_dataset_sizes(self):
        for size in (1000, 100000, 1000000):
            with self.subTest(size=size):
                self.assert_equivalent(generate_dataset(size), 1.5)

    def test_empty_dataset(self):
        for implementation in (analyze, data_analysis.analyze):
            with self.assertRaises(ValueError):
                implementation([])

    def test_invalid_values(self):
        cases = [([1, "2"], TypeError), ([True], TypeError),
                 ([None], TypeError), ([1 + 2j], TypeError),
                 ([float("nan")], ValueError), ([float("inf")], ValueError),
                 (None, TypeError), ([10 ** 1000], OverflowError),
                 ([-1e308, 1e308], OverflowError)]
        for values, error in cases:
            for implementation in (analyze, data_analysis.analyze):
                with self.subTest(values=values, implementation=implementation.__module__):
                    with self.assertRaises(error):
                        implementation(values)

    def test_invalid_thresholds(self):
        cases = [(True, TypeError), ("2", TypeError), (None, TypeError),
                 (-1, ValueError), (float("inf"), ValueError),
                 (float("nan"), ValueError)]
        for threshold, error in cases:
            for implementation in (analyze, data_analysis.analyze):
                with self.subTest(threshold=threshold, implementation=implementation.__module__):
                    with self.assertRaises(error):
                        implementation([1, 2, 3], threshold)

    def test_keyword_arguments(self):
        expected = analyze(values=[1, 2, 3], threshold=1)
        actual = data_analysis.analyze(values=[1, 2, 3], threshold=1)
        self.assertEqual(actual, expected)

    def test_input_unchanged(self):
        for implementation in (analyze, data_analysis.analyze):
            values = [3, 1, 2]
            implementation(values)
            self.assertEqual(values, [3, 1, 2])


if __name__ == "__main__":
    unittest.main(verbosity=2)
