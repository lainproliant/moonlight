#!/usr/bin/env python
# --------------------------------------------------------------------
# build.py
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Thursday December 10, 2020
#
# Distributed under terms of the MIT license.
# -------------------------------------------------------------------

import os
import random
import shlex
from pathlib import Path

from xeno.build import build, default, factory, provide, sh, target, Recipe
from xeno.shell import check

INTERACTIVE_TESTS = {"ansi"}

INCLUDES = ["-I./include",
            "-I./deps/date/include",
            "-I./deps/tinyformat",
            "-I./deps/sole",
            "-I./deps/nanoid"]

ENV = dict(
    CC=os.environ.get("CC", "clang++"),
    CFLAGS=(
        *shlex.split(os.environ.get("CFLAGS", "")),
        "-g",
        "-fpermissive",  # needed for g++ to respect "always_false<T>"
        *INCLUDES,
        "-DMOONLIGHT_ENABLE_STACKTRACE",
        "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
        "--std=c++2a",
    ),
    LDFLAGS=(
        *shlex.split(os.environ.get("LDFLAGS", "")),
        "-g", "-lpthread"),
)

# -------------------------------------------------------------------
@provide
def submodules():
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@factory
def compile(src, headers):
    cmd = "{CC} {CFLAGS} {src} {LDFLAGS} -o {output}"

    return sh(
        cmd,
        env=ENV,
        src=src,
        output=Path(src).with_suffix(""),
        requires=headers,
    )


# -------------------------------------------------------------------
@factory
def test(test):
    return sh(
        "{test}",
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
    return [compile(src, headers) for src in lab_sources]


# -------------------------------------------------------------------
@target
async def tests(test_sources, headers, submodules):
    await submodules.resolve()
    tests = [compile(src, headers) for src in test_sources]
    return [*random.sample(tests, len(tests))]

# -------------------------------------------------------------------
@default
def run_tests(tests):
    return (test(t) for t in tests)

# -------------------------------------------------------------------
@target
def all(tests, labs):
    return (tests, labs)

# -------------------------------------------------------------------
@target
def cc_json():
    return sh("intercept-build ./build.py compile:\* -R; ./build.py -c compile:\*")

# -------------------------------------------------------------------
if __name__ == "__main__":
    build()
