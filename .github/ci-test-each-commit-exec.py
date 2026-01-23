#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import argparse
import subprocess
import sys
import shlex


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    try:
        return subprocess.run(cmd, check=True, **kwargs)
    except Exception as e:
        sys.exit(e)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--skip-unit-tests", action="store_true", help="Skip CMake/ctest unit tests")
    parser.add_argument("--skip-functional-tests", action="store_true", help="Skip functional tests")
    parser.add_argument("--failfast", action="store_true", help="Stop functional tests after the first failure")
    args = parser.parse_args()

    print("Running tests on commit ...")
    run(["git", "log", "-1"])

    num_procs = int(run(["nproc"], stdout=subprocess.PIPE).stdout)
    build_dir = "ci_build"

    run([
        "cmake",
        "-B",
        build_dir,
        "-Werror=dev",
        # Use clang++, because it is a bit faster and uses less memory than g++
        "-DCMAKE_C_COMPILER=clang",
        "-DCMAKE_CXX_COMPILER=clang++",
        # Use mold, because it is faster than the default linker
        "-DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=mold",
        # Use Debug build type for more debug checks, but enable optimizations
        "-DAPPEND_CXXFLAGS='-O3 -g2'",
        "-DAPPEND_CFLAGS='-O3 -g2'",
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DWERROR=ON",
        "--preset=dev-mode",
        # Tolerate unused member functions in intermediate commits in a pull request
        "-DCMAKE_CXX_FLAGS=-Wno-error=unused-member-function",
    ])
    run(["cmake", "--build", build_dir, "-j", str(num_procs)])
    if not args.skip_unit_tests:
        run([
            "ctest",
            "--output-on-failure",
            "--stop-on-failure",
            "--test-dir",
            build_dir,
            "-j",
            str(num_procs),
        ])

    if not args.skip_functional_tests:
        cmd = [
            sys.executable,
            f"./{build_dir}/test/functional/test_runner.py",
            "-j",
            str(num_procs * 2),
            "--combinedlogslen=99999999",
        ]
        if args.failfast:
            cmd.append("--failfast")
        run(cmd)


if __name__ == "__main__":
    main()
