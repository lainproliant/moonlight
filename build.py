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
from pathlib import Path

from xeno.build import build, engine, provide, task
from xeno.recipes import checkout, install, sh, test
from xeno.recipes.cxx import ENV, compile

# -------------------------------------------------------------------
DEPS = [
    "https://github.com/HowardHinnant/date",
    "https://github.com/c42f/tinyformat",
    "https://github.com/mariusbancila/stduuid",
    "https://github.com/tplgy/cppcodec",
]

INTERACTIVE_TESTS = {"ansi"}

INCLUDES = [
    "-I./include",
    "-I./deps/date/include",
    "-I./deps",
    "-I./deps/stduuid/include",
    "-I./deps/cppcodec",
]

ENV.update(
    append="CFLAGS,LDFLAGS",
    CFLAGS=[
        "-Wall",
        "-fpermissive",  # needed for g++ to respect "always_false<T>"
        *INCLUDES,
        "-DMOONLIGHT_ENABLE_STACKTRACE",
        "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
        "--std=c++23",
        "-O3"
    ],
    LDFLAGS=["-O3", "-lpthread", "-lutil"],
    PREFIX=os.environ.get("PREFIX", "/usr/local"),
    DESTDIR=os.environ.get("DESTDIR", ""),
)


# --------------------------------------------------------------------
@task(keep=True)
def deps():
    """Fetch third-party repos."""
    return [checkout(repo) for repo in DEPS]


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
@task(dep="deps")
def labs(lab_sources, headers, deps):
    """Compile labs programs, little demos for testing features out."""
    return [compile(src, headers=headers) for src in lab_sources]


# -------------------------------------------------------------------
@task(dep="deps")
def tests(test_sources, headers, deps):
    """Compile and shuffle the unit tests."""
    tests = [compile(src, headers=headers) for src in test_sources]
    return [*random.sample(tests, len(tests))]


# -------------------------------------------------------------------
@task
def utils(util_sources, headers):
    """Build util programs."""
    utils = [compile(src, headers=headers) for src in util_sources]
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
    return [test(t, interactive=INTERACTIVE_TESTS) for t in tests]


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
    build()
