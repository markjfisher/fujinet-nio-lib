#!/usr/bin/env python3
"""
FujiNet-NIO Library Example Test Suite

Runs all example applications against both POSIX and ESP32 targets.
Builds the library and examples fresh before running.

Usage:
    python suite.py --posix-port /dev/pts/2 --esp32-port /dev/ttyUSB0 --services-ip 192.168.1.101

Requirements:
    - Test services must be running (./scripts/start_test_services.sh all from fujinet-nio)
    - ESP32 must be flashed with fujinet-nio firmware
"""

import argparse
import os
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Tuple


def get_default_ip() -> str:
    """Get the default non-loopback IP address of this machine."""
    try:
        # Create a socket to determine the outbound IP
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            # Connect to an external address (doesn't actually send data)
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except Exception:
        return "127.0.0.1"


@dataclass
class TestResult:
    name: str
    target: str
    passed: bool
    output: str
    duration_ms: float


class TestSuite:
    def __init__(self, posix_port: str, esp32_port: str, services_ip: str, verbose: bool = False):
        self.posix_port = posix_port
        self.esp32_port = esp32_port
        self.services_ip = services_ip
        self.verbose = verbose
        self.examples_dir = Path(__file__).parent
        self.results: List[TestResult] = []

    def run_cmd(self, cmd: List[str], env: Optional[dict] = None, timeout: int = 30) -> Tuple[int, str, float]:
        """Run a command and return (exit_code, output, duration_ms)."""
        start = time.time()
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout,
                env={**os.environ, **(env or {})}
            )
            duration_ms = (time.time() - start) * 1000
            output = result.stdout + result.stderr
            return result.returncode, output, duration_ms
        except subprocess.TimeoutExpired:
            duration_ms = (time.time() - start) * 1000
            return -1, "TIMEOUT", duration_ms
        except Exception as e:
            duration_ms = (time.time() - start) * 1000
            return -1, str(e), duration_ms

    def build_all(self, target: str) -> bool:
        """Build library and all examples for target."""
        print(f"\n{'='*60}")
        print(f"Building for {target.upper()}")
        print(f"{'='*60}")

        # Build library first (use specific target like 'make linux', not 'make TARGET=linux')
        lib_dir = self.examples_dir.parent
        print(f"\n[1/2] Building library for {target}...")
        code, out, _ = self.run_cmd(["make", target, "-C", str(lib_dir)], timeout=120)
        if code != 0:
            print(f"FAILED: Library build failed for {target}")
            if self.verbose:
                print(out)
            return False
        print(f"  ✓ Library built")

        # Build examples
        print(f"\n[2/2] Building examples for {target}...")
        code, out, _ = self.run_cmd(["make", f"TARGET={target}", "-C", str(self.examples_dir)], timeout=60)
        if code != 0:
            print(f"FAILED: Examples build failed for {target}")
            if self.verbose:
                print(out)
            return False
        print(f"  ✓ Examples built")

        return True

    def run_example(self, name: str, target: str, env: dict, expect_contains: List[str], timeout: int = 30) -> TestResult:
        """Run a single example and check output."""
        bin_path = self.examples_dir / "bin" / target / name

        if not bin_path.exists():
            return TestResult(name, target, False, f"Binary not found: {bin_path}", 0)

        code, output, duration_ms = self.run_cmd([str(bin_path)], env=env, timeout=timeout)

        # Check for expected strings
        passed = True
        for expected in expect_contains:
            if expected not in output:
                passed = False
                output += f"\n[MISSING EXPECTED: {expected}]"

        # Check exit code (0 is success)
        if code != 0:
            passed = False
            output += f"\n[EXIT CODE: {code}]"

        return TestResult(name, target, passed, output, duration_ms)

    def run_clock_test(self, target: str, port: str) -> TestResult:
        """Run clock test example."""
        env = {"FN_PORT": port}
        return self.run_example(
            "clock_test", target, env,
            expect_contains=["FujiNet-NIO Clock", "Current time:"],
            timeout=10
        )

    def run_http_get(self, target: str, port: str, http_host: str) -> TestResult:
        """Run HTTP GET example."""
        env = {
            "FN_PORT": port,
            "FN_TEST_URL": f"http://{http_host}:8080/get"
        }
        return self.run_example(
            "http_get", target, env,
            expect_contains=["HTTP GET", "bytes read"],
            timeout=15
        )

    def run_https_get(self, target: str, port: str, http_host: str) -> TestResult:
        """Run HTTPS GET example with test CA."""
        env = {
            "FN_PORT": port,
            "FN_TEST_URL": f"https://{http_host}:8443/get?testca=1"
        }
        return self.run_example(
            "http_get", target, env,
            expect_contains=["HTTP GET", "bytes read"],
            timeout=15
        )

    def run_tcp_get(self, target: str, port: str, tcp_host: str) -> TestResult:
        """Run TCP GET example."""
        env = {
            "FN_PORT": port,
            "FN_TCP_HOST": tcp_host,
            "FN_TCP_PORT": "7777"
        }
        return self.run_example(
            "tcp_get", target, env,
            expect_contains=["TCP", "Connection established", "Hello"],
            timeout=15
        )

    def run_tls_get(self, target: str, port: str, tcp_host: str) -> TestResult:
        """Run TLS GET example with test CA."""
        env = {
            "FN_PORT": port,
            "FN_TEST_URL": f"tls://{tcp_host}:7778?testca=1"
        }
        return self.run_example(
            "tcp_get", target, env,
            expect_contains=["Connection established", "Hello"],
            timeout=15
        )

    def run_tcp_stream(self, target: str, port: str, tcp_host: str) -> TestResult:
        """Run TCP streaming example."""
        env = {
            "FN_PORT": port,
            "FN_TCP_HOST": tcp_host,
            "FN_TCP_PORT": "7777"
        }
        return self.run_example(
            "tcp_stream", target, env,
            expect_contains=["TCP Streaming", "Connected", "Statistics"],
            timeout=15
        )

    def run_target_tests(self, target: str, port: str, host_ip: str) -> List[TestResult]:
        """Run all tests for a specific target."""
        results = []

        print(f"\n{'='*60}")
        print(f"Running tests for {target.upper()}")
        print(f"  Port: {port}")
        print(f"  Host IP: {host_ip}")
        print(f"{'='*60}")

        tests = [
            ("Clock", lambda: self.run_clock_test(target, port)),
            ("HTTP", lambda: self.run_http_get(target, port, host_ip)),
            ("HTTPS", lambda: self.run_https_get(target, port, host_ip)),
            ("TCP", lambda: self.run_tcp_get(target, port, host_ip)),
            ("TLS", lambda: self.run_tls_get(target, port, host_ip)),
            ("TCP Stream", lambda: self.run_tcp_stream(target, port, host_ip)),
        ]

        for test_name, test_func in tests:
            print(f"\n  [{test_name}] ", end="", flush=True)
            result = test_func()
            results.append(result)

            if result.passed:
                print(f"✓ PASSED ({result.duration_ms:.0f}ms)")
            else:
                print(f"✗ FAILED ({result.duration_ms:.0f}ms)")
                if self.verbose:
                    print(f"    Output: {result.output[:500]}")

        return results

    def run_all(self) -> bool:
        """Run all tests for all targets."""
        all_passed = True

        # Build and test POSIX
        if self.posix_port:
            if not self.build_all("linux"):
                print("\n✗ POSIX build failed, skipping tests")
                all_passed = False
            else:
                # For POSIX, use localhost
                results = self.run_target_tests("linux", self.posix_port, "127.0.0.1")
                self.results.extend(results)
                if not all(r.passed for r in results):
                    all_passed = False

        # Build and test ESP32
        if self.esp32_port and self.services_ip:
            if not self.build_all("linux"):  # Examples are linux binaries that talk to ESP32
                print("\n✗ ESP32 build failed, skipping tests")
                all_passed = False
            else:
                # For ESP32, use the services IP (where httpbin/socat are running)
                results = self.run_target_tests("linux", self.esp32_port, self.services_ip)
                # Mark these as ESP32 tests in results
                for r in results:
                    r.target = "esp32"
                self.results.extend(results)
                if not all(r.passed for r in results):
                    all_passed = False

        return all_passed

    def print_summary(self):
        """Print test summary."""
        print(f"\n{'='*60}")
        print("TEST SUMMARY")
        print(f"{'='*60}")

        # Group by target
        targets = sorted(set(r.target for r in self.results))

        total_passed = 0
        total_failed = 0

        for target in targets:
            target_results = [r for r in self.results if r.target == target]
            passed = sum(1 for r in target_results if r.passed)
            failed = sum(1 for r in target_results if not r.passed)
            total_passed += passed
            total_failed += failed

            print(f"\n{target.upper()}:")
            for r in target_results:
                status = "✓" if r.passed else "✗"
                print(f"  {status} {r.name} ({r.duration_ms:.0f}ms)")
            print(f"  Total: {passed} passed, {failed} failed")

        print(f"\n{'='*60}")
        print(f"OVERALL: {total_passed} passed, {total_failed} failed")
        print(f"{'='*60}")

        if total_failed > 0:
            print("\nFailed tests:")
            for r in self.results:
                if not r.passed:
                    print(f"\n[{r.target}] {r.name}:")
                    print(f"  {r.output[:500]}")


def main():
    default_ip = get_default_ip()

    parser = argparse.ArgumentParser(
        description="Run FujiNet-NIO library examples against POSIX and ESP32 targets",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
    # Run against POSIX only
    python suite.py --posix-port /dev/pts/2

    # Run against ESP32 only  
    python suite.py --esp32-port /dev/ttyUSB0

    # Run against both with explicit services IP
    python suite.py --posix-port /dev/pts/2 --esp32-port /dev/ttyUSB0 --services-ip 192.168.1.101

Prerequisites:
    - Test services must be running: ./scripts/start_test_services.sh all
    - ESP32 must be flashed with fujinet-nio firmware
        """
    )

    parser.add_argument("-p", "--posix-port", type=str, default="",
                        help="Serial port for POSIX target (e.g., /dev/pts/2)")
    parser.add_argument("-e", "--esp32-port", type=str, default="",
                        help="Serial port for ESP32 target (e.g., /dev/ttyUSB0)")
    parser.add_argument("-s", "--services-ip", type=str, default=default_ip,
                        help=f"IP address where test services are running (default: {default_ip})")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Show verbose output")

    args = parser.parse_args()

    if not args.posix_port and not args.esp32_port:
        parser.error("At least one of --posix-port or --esp32-port is required")

    suite = TestSuite(
        posix_port=args.posix_port,
        esp32_port=args.esp32_port,
        services_ip=args.services_ip,
        verbose=args.verbose
    )

    success = suite.run_all()
    suite.print_summary()

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
