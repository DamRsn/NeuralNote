#!/usr/bin/env python3
"""Configures and builds NeuralNote in Release for CI, through ccache.

macOS builds one architecture, given by --arch. For x86_64, MUSCRIPTOR_LLVM_OBJCOPY
in the environment, when set, names the llvm-objcopy the CPU variants use. Windows
builds with clang-cl and lld-link, taken from CC, CXX, RC, CMAKE_LINKER and
CMAKE_AR in the environment (see windows_tools.py).

The output is in build/NeuralNote_artefacts/Release.
"""

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD_DIR = ROOT / "build"


def run(*command: str) -> None:
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True)


def launchers(ccache: str, languages: list[str]) -> list[str]:
    return [f"-DCMAKE_{language}_COMPILER_LAUNCHER={ccache}" for language in languages]


def macos_options(arch: str, ccache: str) -> tuple[list[str], list[str]]:
    options = [
        f"-DCMAKE_OSX_ARCHITECTURES={arch}",
        *launchers(ccache, ["C", "CXX", "OBJC", "OBJCXX"]),
        "-DMUSCRIPTOR_METAL=ON",
        "-DMUSCRIPTOR_METAL_PRECOMPILED=ON",
    ]
    targets = ["NeuralNote_Standalone", "NeuralNote_VST3", "NeuralNote_AU"]

    if arch == "x86_64":
        # ggml-cpu per instruction-set level, picked at run time.
        options.append("-DMUSCRIPTOR_CPU_VARIANTS=ON")

        if objcopy := os.environ.get("MUSCRIPTOR_LLVM_OBJCOPY"):
            options.append(f"-DMUSCRIPTOR_LLVM_OBJCOPY={objcopy}")

        # Rosetta runs AVX2 without advertising it, so only this check catches it outside the variants.
        targets.append("muscriptor_check_cpu_baseline")
    else:
        # The fixed ARM baseline, never the runner's own CPU.
        options.append("-DMUSCRIPTOR_NATIVE=OFF")

    return options, targets


def windows_options(ccache: str) -> tuple[list[str], list[str]]:
    env = os.environ
    options = [
        # CMake takes the compilers from CC, CXX and RC, but not these. LTO objects are LLVM
        # bitcode, which only lld-link and llvm-lib read.
        f"-DCMAKE_LINKER={env['CMAKE_LINKER']}",
        f"-DCMAKE_AR={env['CMAKE_AR']}",
        *launchers(ccache, ["C", "CXX"]),
        "-DNEURALNOTE_ASIO=ON",
        # ggml-cpu per instruction-set level, picked at run time.
        "-DMUSCRIPTOR_CPU_VARIANTS=ON",
        # Fails the configure if the SDK is missing, rather than building without the backend.
        "-DMUSCRIPTOR_VULKAN=ON",
    ]

    return options, ["NeuralNote_Standalone", "NeuralNote_VST3", "muscriptor_check_cpu_baseline"]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--arch", choices=["arm64", "x86_64"], help="macOS architecture to build")
    args = parser.parse_args()

    ccache = os.environ.get("CCACHE_EXE") or shutil.which("ccache")

    if not ccache:
        raise SystemExit("ccache not found: set CCACHE_EXE or put it on PATH")

    if sys.platform == "darwin":
        if not args.arch:
            parser.error("--arch is required on macOS")

        options, targets = macos_options(args.arch, ccache)
    elif sys.platform == "win32":
        options, targets = windows_options(ccache)
    else:
        raise SystemExit(f"Unsupported platform: {sys.platform}")

    run(
        "cmake", "-S", str(ROOT), "-B", str(BUILD_DIR), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DLTO=ON",
        "-DNEURALNOTE_SYMBOLS=ON",
        "-DMUSCRIPTOR_BUILD_TESTS=OFF",
        *options,
    )

    run(ccache, "--zero-stats")
    run("cmake", "--build", str(BUILD_DIR), "--target", *targets)
    run(ccache, "--show-stats")


if __name__ == "__main__":
    main()
