#!/usr/bin/env python3
"""Configures and builds NeuralNote in Release for CI, through ccache.

macOS builds one architecture, given by --arch. Windows builds with clang-cl and
lld-link, taken from CC, CXX, RC, CMAKE_LINKER and CMAKE_AR in the environment
(see windows_tools.py).

On Windows, --cpu-target selects the instruction set:
  generic   - baseline x86-64 (SSE2), same as before
  braswell  - Intel Braswell / Airmont (Silvermont-class): SSE4.2, POPCNT, AES,
              PCLMUL, RDRAND, but no AVX/AVX2/FMA/F16C/BMI

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

# ggml SIMD switches. For Braswell we want SSE4.2 and friends ON, everything
# AVX-class OFF. Setting them explicitly matters because GGML_NATIVE=OFF alone
# can still enable AVX by default (see ggml's CMakeLists INS_ENB logic).
GGML_BRASWELL_FLAGS = [
    "-DGGML_NATIVE=OFF",
    "-DGGML_SSE42=ON",
    "-DGGML_AVX=OFF",
    "-DGGML_AVX2=OFF",
    "-DGGML_AVX512=OFF",
    "-DGGML_AVX512_VBMI=OFF",
    "-DGGML_AVX512_VNNI=OFF",
    "-DGGML_AVX512_BF16=OFF",
    "-DGGML_FMA=OFF",
    "-DGGML_F16C=OFF",
    "-DGGML_BMI2=OFF",
    "-DGGML_AVX_VNNI=OFF",
    "-DGGML_AVX_VNNI_INT8=OFF",
    "-DGGML_AVX_VNNI_INT16=OFF",
    "-DGGML_CPU_ALL_VARIANTS=OFF",
    "-DGGML_BACKEND_DL=OFF",
]

# -march=silvermont is what GCC/Clang use for Airmont/Braswell; there is no
# -march=airmont or -march=braswell. It emits SSE4.2/POPCNT/AES/PCLMUL/RDRAND
# and forbids AVX/AVX2/FMA/F16C/BMI.
BRASWELL_CFLAGS = "-march=silvermont"


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

    return options, ["NeuralNote_Standalone", "NeuralNote_VST3", "NeuralNote_AU"]


def windows_options(ccache: str, cpu_target: str) -> tuple[list[str], list[str]]:
    env = os.environ
    options = [
        # CMake takes the compilers from CC, CXX and RC, but not these. LTO objects are LLVM
        # bitcode, which only lld-link and llvm-lib read.
        f"-DCMAKE_LINKER={env['CMAKE_LINKER']}",
        f"-DCMAKE_AR={env['CMAKE_AR']}",
        *launchers(ccache, ["C", "CXX"]),
        "-DNEURALNOTE_ASIO=ON",
        # Fails the configure if the SDK is missing, rather than building without the backend.
        "-DMUSCRIPTOR_VULKAN=ON",
    ]

    if cpu_target == "braswell":
        options += [
            *GGML_BRASWELL_FLAGS,
            f"-DCMAKE_C_FLAGS={BRASWELL_CFLAGS}",
            f"-DCMAKE_CXX_FLAGS={BRASWELL_CFLAGS}",
        ]

    return options, ["NeuralNote_Standalone", "NeuralNote_VST3"]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--arch", choices=["arm64", "x86_64"], help="macOS architecture to build")
    parser.add_argument(
        "--cpu-target",
        choices=["generic", "braswell"],
        default="generic",
        help="Windows CPU target (default: generic, i.e. baseline x86-64)",
    )
    args = parser.parse_args()

    ccache = os.environ.get("CCACHE_EXE") or shutil.which("ccache")

    if not ccache:
        raise SystemExit("ccache not found: set CCACHE_EXE or put it on PATH")

    if sys.platform == "darwin":
        if not args.arch:
            parser.error("--arch is required on macOS")

        options, targets = macos_options(args.arch, ccache)
    elif sys.platform == "win32":
        options, targets = windows_options(ccache, args.cpu_target)
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
