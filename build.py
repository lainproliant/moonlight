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
import sys
import random
import shlex
from pathlib import Path

from xeno.build import provide, recipe, task, engine
from xeno.cookbook import sh

INTERACTIVE_TESTS = {"ansi"}

INCLUDES = ["-I./include", "-I./deps/date/include", "-I./deps"]

ENV = dict(
    CC=os.environ.get("CC", "clang++"),
    CFLAGS=shlex.join(
        [
            *shlex.split(os.environ.get("CFLAGS", "")),
            "-Wall",
            "-g",
            "-fpermissive",  # needed for g++ to respect "always_false<T>"
            *INCLUDES,
            "-DMOONLIGHT_ENABLE_STACKTRACE",
            "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
            "--std=c++2a",
        ]
    ),
    LDFLAGS=(
        shlex.join([*shlex.split(os.environ.get("LDFLAGS", "")), "-g", "-lpthread"])
    ),
    PREFIX=os.environ.get("PREFIX", "/usr/local"),
    DESTDIR=os.environ.get("DESTDIR", ""),
    STRESS_CYCLES=os.environ.get("STRESS_CYCLES", "100"),
)


# -------------------------------------------------------------------
@recipe(sigil=lambda r: f'{r.name}:{r.arg("program").target.name}')
async def stress_test(program, cycles: int = 10):
    for x in range(cycles):
        await sh(program)()


# -------------------------------------------------------------------
@task
def submodules():
    """Fetch git submodules needed for compilation."""
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@recipe(factory=True, sigil=lambda r: f'{r.name}:{r.target.name}')
def compile(src, headers):
    cmd = "{CC} {CFLAGS} {src} {LDFLAGS} -o {target}"

    return sh(cmd, env=ENV, src=src, target=src.with_suffix(""))


# -------------------------------------------------------------------
@recipe(factory=True, sigil=lambda r: f"{r.name}:{r.arg('t').target.name}")
def test(t):
    result = sh(
        t.target,
        t=t,
        env=ENV,
        interactive=t.name in INTERACTIVE_TESTS,
    )
    return result


# -------------------------------------------------------------------
@recipe(factory=True)
def install(program):
    path = Path(ENV["DESTDIR"]) / ENV["PREFIX"] / "bin" / program.target.name
    return sh(
        "mkdir -p {DESTDIR}{PREFIX}/bin; "
        "cp -f {program} {target}; "
        "chmod 775 {target}",
        env=ENV,
        program=program,
        target=path,
        as_user="root",
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
def util_sources():
    return Path.cwd().glob("utils/*.cpp")


# -------------------------------------------------------------------
@provide
def headers():
    return Path.cwd().glob("include/moonlight/*.h")


# -------------------------------------------------------------------
@task(dep="submodules")
def labs(lab_sources, headers, submodules):
    """Compile labs programs, little demos for testing features out."""
    return [compile(src, headers) for src in lab_sources]


# -------------------------------------------------------------------
@task(dep="submodules")
def tests(test_sources, headers, submodules):
    """Compile and shuffle the unit tests."""
    tests = [compile(src, headers) for src in test_sources]
    return [*random.sample(tests, len(tests))]


# -------------------------------------------------------------------
@task
def utils(util_sources, headers, submodules):
    """Build util programs."""
    utils = [compile(src, headers) for src in util_sources]
    return utils


# -------------------------------------------------------------------
@task
def install_utils(utils):
    """Install the util programs to the system."""
    return (install(util) for util in utils)


# -------------------------------------------------------------------
@task(default=True)
def run_tests(tests):
    """Run all of the unit tests."""
    return [test(t) for t in tests]


# -------------------------------------------------------------------
@task
def assert_tests_stable(tests):
    """Repeatedly run all of the unit tests to assert they are stable."""
    return (stress_test(t) for t in tests)


# -------------------------------------------------------------------
@task
def all(tests, labs, utils):
    """Compile tests, labs, and utils."""
    return (tests, labs, utils)


# -------------------------------------------------------------------
@task
def cc_json():
    """Generate compile_commands.json for IDEs."""
    return sh("intercept-build ./build.py compile:\\* -R; ./build.py -c compile:\\*")


# -------------------------------------------------------------------
if __name__ == "__main__":
    engine.name = "Build Script for Moonlight C++"
    engine.build(*sys.argv[1:])
