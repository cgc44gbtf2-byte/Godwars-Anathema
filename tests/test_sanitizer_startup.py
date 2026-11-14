"""A diagnostic PIE timeout must not mask configured sanitizer failures."""
import subprocess
import unittest
from unittest.mock import Mock

from sanitizer_startup import diagnose_pie, require_detection, require_startups


def completed(code=0, stderr=b""):
    return subprocess.CompletedProcess(["probe"], code, b"", stderr)


class SanitizerStartupTests(unittest.TestCase):
    def test_diagnostic_successes_are_counted(self):
        run = Mock(return_value=completed())
        self.assertEqual(diagnose_pie(run, "pie"), {
            "attempts": 50, "unsuccessful": 0, "timeouts": 0})
        self.assertEqual(run.call_count, 50)

    def test_diagnostic_crashes_are_reported(self):
        run = Mock(side_effect=[completed(-11), completed(0)])
        self.assertEqual(diagnose_pie(run, "pie", 2), {
            "attempts": 2, "unsuccessful": 1, "timeouts": 0})

    def test_first_diagnostic_timeout_stops_only_the_diagnostic(self):
        run = Mock(side_effect=[completed(), subprocess.TimeoutExpired("pie", 15)])
        self.assertEqual(diagnose_pie(run, "pie"), {
            "attempts": 2, "unsuccessful": 0, "timeouts": 1})
        self.assertEqual(run.call_count, 2)

    def test_configured_startup_requires_all_100_successes(self):
        run = Mock(return_value=completed())
        require_startups(run, "fixed")
        self.assertEqual(run.call_count, 100)

    def test_configured_crash_fails_immediately_without_retry(self):
        run = Mock(return_value=completed(-11, b"AddressSanitizer:DEADLYSIGNAL"))
        with self.assertRaises(AssertionError):
            require_startups(run, "fixed")
        self.assertEqual(run.call_count, 1)

    def test_configured_timeout_is_not_caught_or_retried(self):
        run = Mock(side_effect=subprocess.TimeoutExpired("fixed", 15))
        with self.assertRaises(subprocess.TimeoutExpired):
            require_startups(run, "fixed")
        self.assertEqual(run.call_count, 1)

    def test_detection_requires_failure_and_the_expected_report(self):
        for value in (completed(0, b"heap-buffer-overflow"),
                      completed(1, b"unrelated failure")):
            with self.assertRaises(AssertionError):
                require_detection(Mock(return_value=value), "fixed",
                                  b"heap-buffer-overflow")

    def test_asan_and_ubsan_reports_are_required(self):
        for message, args in ((b"heap-buffer-overflow", ()),
                              (b"signed integer overflow", ("ubsan",))):
            run = Mock(return_value=completed(1, message))
            require_detection(run, "fixed", message, *args)
            run.assert_called_once_with("fixed", *args)


if __name__ == "__main__":
    unittest.main()
