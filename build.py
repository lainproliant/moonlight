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

from xeno.build import Recipe, build, default, action, provide, sh, target

INTERACTIVE_TESTS = {"ansi"}

INCLUDES = ["-I./include", "-I./deps/date/include", "-I./deps"]

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
    LDFLAGS=(*shlex.split(os.environ.get("LDFLAGS", "")), "-g", "-lpthread"),
    PREFIX=os.environ.get("PREFIX", "/usr/local"),
    DESTDIR=os.environ.get("DESTDIR", ""),
    STRESS_CYCLES=os.environ.get("STRESS_CYCLES", "100"),
)

# -------------------------------------------------------------------
class StressTest(Recipe):
    def __init__(self, input, cycles=ENV["STRESS_CYCLES"]):
        super().__init__([input])
        self.named(input.name).with_type("stress")
        self._cycles = int(cycles)
        self._done = False
        self._test = input

    async def make(self):
        for x in range(self._cycles):
            test = sh("{test}", test=self._test, interactive=True)
            await test.resolve()
            assert test.done, f"Stress test failed after {x} iterations."
        self._done = True

    @property
    def done(self):
        return self._done

# -------------------------------------------------------------------
@provide
def submodules():
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@action
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
@action
def test(test):
    return sh(
        "{test}",
        env=ENV,
        test=test,
        interactive=test.output.name in INTERACTIVE_TESTS,
    )


# -------------------------------------------------------------------
@action
def install(program):
    output = ENV["DESTDIR"] + ENV["PREFIX"] + "/bin/" + program.output.name
    return sh(
        "mkdir -p {DESTDIR}{PREFIX}/bin; "
        "cp -f {program} {output}; "
        "chmod 775 {output}",
        env=ENV,
        program=program,
        output=output,
        as_user="root",
    ).named(program.name)


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
def util_sources():
    return Path.cwd().glob("utils/*.cpp")


# -------------------------------------------------------------------
@provide
def headers():
    return Path.cwd().glob("include/moonlight/*.h")


# -------------------------------------------------------------------
@target
async def labs(lab_sources, headers, submodules):
    await submodules.resolve()
    return [compile(src, headers).with_prefix("lab.") for src in lab_sources]


# -------------------------------------------------------------------
@target
async def tests(test_sources, headers, submodules):
    await submodules.resolve()
    tests = [compile(src, headers).with_prefix("test.") for src in test_sources]
    return [*random.sample(tests, len(tests))]


# -------------------------------------------------------------------
@target
async def utils(util_sources, headers, submodules):
    await submodules.resolve()
    utils = [compile(src, headers).with_prefix("util.") for src in util_sources]
    return utils


# -------------------------------------------------------------------
@target
def install_utils(utils):
    return (install(util) for util in utils)


# -------------------------------------------------------------------
@default
def run_tests(tests):
    return (test(t) for t in tests)


# -------------------------------------------------------------------
@target
def assert_tests_stable(tests):
    return (StressTest(t) for t in tests)


# -------------------------------------------------------------------
@target
def all(tests, labs, utils, assert_tests_stable):
    return (tests, labs)


# -------------------------------------------------------------------
@target
def cc_json():
    return sh("intercept-build ./build.py compile:\* -R; ./build.py -c compile:\*")


# -------------------------------------------------------------------
if __name__ == "__main__":
    build()
