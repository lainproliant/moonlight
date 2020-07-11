# --------------------------------------------------------------------
# bake.py: Build steps for moonlight unit tests.
#
# Author: Lain Musgrove (lain.proliant@gmail.com)
# Date: Sunday January 5, 2020
# --------------------------------------------------------------------
#
from pathlib import Path
from panifex import build, sh, default, target, provide

# -------------------------------------------------------------------
sh.env(
    CC="g++",
    CFLAGS=(
        "-g",
        "-I./include",
        "--std=c++2a",
        "-DMOONLIGHT_DEBUG",
        "-DMOONLIGHT_ENABLE_STACKTRACE",
        "-DMOONLIGHT_STACKTRACE_IN_DESCRIPTION",
    ),
    LDFLAGS=("-rdynamic", "-g", "-lpthread", "-ldl"),
)


# -------------------------------------------------------------------
def compile_test(src):
    return sh(
        "{CC} {CFLAGS} {input} {LDFLAGS} -o {output}",
        input=src,
        output=Path(src).with_suffix(""),
    )


# -------------------------------------------------------------------
@provide
def submodules():
    return sh("git submodule update --init --recursive")


# -------------------------------------------------------------------
@provide
def sources(submodules):
    return Path.cwd().glob("test/*.cpp")


# -------------------------------------------------------------------
@provide
def tests(sources):
    return [compile_test(src) for src in sources]


# -------------------------------------------------------------------
@target
def run_tests(tests):
    return (sh("{input}", input=test, cwd="test").interactive() for test in tests)


# -------------------------------------------------------------------
@default
def run_tests_async(tests):
    return (sh("{input}", input=test, cwd="test") for test in tests)


# -------------------------------------------------------------------
build()
