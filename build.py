#!/usr/bin/env python
# --------------------------------------------------------------------
# build.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Thursday December 10, 2020
#
# Distributed under terms of the MIT license.
# -------------------------------------------------------------------

import random
from pathlib import Path

from xeno.build import build, default, factory, provide, sh, target

INTERACTIVE_TESTS = {"ansi"}

INCLUDES = ["-I./include", "-I./deps/date/include"]

ENV = dict(
    CC="clang++",
    CFLAGS=(
        "-g",
        *INCLUDES,
        "--std=c++2a",
        "-DMOONLIGHT_DEBUG",
        "-DMOONLIGHT_ENABLE_STACKTRACE",
        "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
    ),
    LDFLAGS=("-rdynamic", "-g", "-ldl", "-lpthread"),
)

# -------------------------------------------------------------------
@provide
def submodules():
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@factory
def compile_test(src, headers):
    return sh(
        "{CC} {CFLAGS} {src} {LDFLAGS} -o {output}",
        env=ENV,
        src=src,
        output=Path(src).with_suffix(""),
        requires=headers,
    )


# -------------------------------------------------------------------
@factory
def run_test(test):
    return sh(
        "{test}",
        cwd="test",
        env=ENV,
        test=test,
        interactive=test.output.name in INTERACTIVE_TESTS,
    )


# -------------------------------------------------------------------
@provide
def test_sources():
    return Path.cwd().glob("test/*.cpp")


# -------------------------------------------------------------------
@provide
def lab_sources():
    return Path.cwd().glob("lab/*.cpp")

# -------------------------------------------------------------------
@provide
def headers():
    return Path.cwd().glob("include/moonlight/*.h")


# -------------------------------------------------------------------
@target
async def labs(lab_sources, headers, submodules):
    await submodules.resolve()
    return [compile_test(src, headers) for src in lab_sources]


# -------------------------------------------------------------------
@target
async def tests(test_sources, headers, submodules):
    await submodules.resolve()
    tests = [compile_test(src, headers) for src in test_sources]
    return random.sample(tests, len(tests))


# -------------------------------------------------------------------
@default
def run_tests(tests):
    return tuple(run_test(test) for test in tests)


# -------------------------------------------------------------------
@target
def all(run_tests, labs):
    return (run_tests, labs)

# -------------------------------------------------------------------
if __name__ == "__main__":
    build()
