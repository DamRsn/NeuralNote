#!/usr/bin/env python3
"""Sets up the Windows toolchain for configure_and_build.py, through GITHUB_ENV.

- The Visual Studio developer environment, for the Windows SDK and its tools.
  Nothing is compiled with cl.
- clang-cl, lld-link, llvm-lib and llvm-rc from the LLVM the runner image ships.
  CC and CXX also reach ggml's nested vulkan-shaders-gen build, which sees only
  the environment.
- The Vulkan SDK in C:\\VulkanSDK\\<version>, installed unless it is already
  there (restored from the cache).
- ccache.

Versions and digests come from VULKAN_SDK_VERSION, VULKAN_SDK_SHA256,
CCACHE_VERSION and CCACHE_SHA256.
"""

import hashlib
import os
import shutil
import subprocess
import urllib.request
import zipfile
from pathlib import Path

LLVM_BIN = Path(r"C:\Program Files\LLVM\bin")
VSWHERE = Path(os.environ["ProgramFiles(x86)"]) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
TEMP = Path(os.environ["RUNNER_TEMP"])


def download(url: str, sha256: str, destination: Path) -> None:
    print(f"Downloading {url}", flush=True)

    # sdk.lunarg.com answers 403 to urllib's default User-Agent.
    request = urllib.request.Request(url, headers={"User-Agent": "NeuralNote-CI"})

    with urllib.request.urlopen(request, timeout=60) as response, open(destination, "wb") as file:
        shutil.copyfileobj(response, file)
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()

    if digest.lower() != sha256.lower():
        raise SystemExit(f"{destination.name}: SHA-256 {digest}, expected {sha256}")


def developer_environment() -> dict[str, str]:
    vs = subprocess.run(
        [str(VSWHERE), "-latest", "-products", "*", "-property", "installationPath"],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
    vcvars = Path(vs) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"

    # A string, so it reaches cmd as written; /s strips the outer quotes.
    output = subprocess.run(
        f'cmd /s /c ""{vcvars}" > nul && set"', capture_output=True, text=True, check=True
    ).stdout

    changed = {}

    for line in output.splitlines():
        name, separator, value = line.partition("=")

        if separator and os.environ.get(name) != value:
            changed[name] = value

    return changed


def install_vulkan_sdk(version: str, sha256: str) -> Path:
    root = Path(rf"C:\VulkanSDK\{version}")

    if (root / "Bin" / "glslc.exe").exists():
        print(f"Vulkan SDK {version} already in {root}")
        return root

    installer = TEMP / "vulkansdk.exe"
    download(
        f"https://sdk.lunarg.com/sdk/download/{version}/windows/vulkansdk-windows-X64-{version}.exe",
        sha256, installer,
    )
    subprocess.run(
        [str(installer), "--root", str(root), "--accept-licenses", "--default-answer", "--confirm-command", "install"],
        check=True,
    )

    if not (root / "Bin" / "glslc.exe").exists():
        raise SystemExit(f"glslc not found in {root} after installing the Vulkan SDK")

    return root


def install_ccache(version: str, sha256: str) -> Path:
    archive = TEMP / "ccache.zip"
    download(
        f"https://github.com/ccache/ccache/releases/download/v{version}/ccache-{version}-windows-x86_64.zip",
        sha256, archive,
    )

    with zipfile.ZipFile(archive) as zip_file:
        zip_file.extractall(TEMP)

    return TEMP / f"ccache-{version}-windows-x86_64" / "ccache.exe"


def llvm_tool(name: str) -> Path:
    tool = LLVM_BIN / f"{name}.exe"

    if not tool.exists():
        raise SystemExit(f"{tool} not found")

    return tool


def main() -> None:
    env = os.environ
    variables = developer_environment()

    variables["VULKAN_SDK"] = str(install_vulkan_sdk(env["VULKAN_SDK_VERSION"], env["VULKAN_SDK_SHA256"]))
    variables["CCACHE_EXE"] = install_ccache(env["CCACHE_VERSION"], env["CCACHE_SHA256"]).as_posix()

    clang_cl = llvm_tool("clang-cl").as_posix()
    variables["CC"] = clang_cl
    variables["CXX"] = clang_cl
    variables["RC"] = llvm_tool("llvm-rc").as_posix()
    variables["CMAKE_LINKER"] = llvm_tool("lld-link").as_posix()
    variables["CMAKE_AR"] = llvm_tool("llvm-lib").as_posix()

    subprocess.run([clang_cl, "--version"], check=True)

    with open(env["GITHUB_ENV"], "a") as github_env:
        for name, value in variables.items():
            github_env.write(f"{name}={value}\n")


if __name__ == "__main__":
    main()
