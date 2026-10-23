#!/usr/bin/env python3
"""Builds the Windows installer from build/NeuralNote_artefacts/Release.

Moves the PDBs out of the release directory first, since the linker writes them
beside the binaries, some inside the VST3 bundle, and the installer copies those
directories whole. Writes NeuralNote_Installer_Windows_x64<SUFFIX>.exe and
NeuralNote_Installer_Windows_x64<SUFFIX>_PDBs.zip to Installers/Windows/Output,
where <SUFFIX> is empty for the generic target and "_braswell" otherwise.
"""

import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RELEASE_DIR = ROOT / "build" / "NeuralNote_artefacts" / "Release"
OUTPUT_DIR = ROOT / "Installers" / "Windows" / "Output"
ISCC = Path(os.environ["ProgramFiles(x86)"]) / "Inno Setup 6" / "ISCC.exe"


def cpu_suffix() -> str:
    """Return "" for the generic build, "_braswell" for the Braswell one."""
    target = os.environ.get("CPU_TARGET", "generic")
    if target not in ("generic", "braswell"):
        raise SystemExit(f"Unknown CPU_TARGET: {target!r}")
    return "" if target == "generic" else f"_{target}"


def collect_pdbs(symbols_dir: Path) -> None:
    pdbs = sorted(RELEASE_DIR.rglob("*.pdb"))

    if not pdbs:
        raise SystemExit(f"No PDBs in {RELEASE_DIR}: build Release with NEURALNOTE_SYMBOLS=ON")

    for pdb in pdbs:
        destination = symbols_dir / pdb.relative_to(RELEASE_DIR)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(pdb, destination)
        print(f"Collected {destination.relative_to(symbols_dir)}")


def main() -> None:
    if not ISCC.exists():
        raise SystemExit(f"Inno Setup not found at {ISCC}")

    suffix = cpu_suffix()
    symbols_dir = Path(os.environ["RUNNER_TEMP"]) / "symbols"
    collect_pdbs(symbols_dir)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    shutil.make_archive(
        str(OUTPUT_DIR / f"NeuralNote_Installer_Windows_x64{suffix}_PDBs"),
        "zip",
        symbols_dir,
    )

    # neuralnote.iss resolves ReleaseDir from its own directory, two levels below the root.
    # OutputBaseFilename inside the .iss defaults to NeuralNote_Installer_Windows_x64;
    # /DOutputSuffix lets us override it without editing the .iss per target.
    subprocess.run(
        [str(ISCC), str(ROOT / "Installers" / "Windows" / "neuralnote.iss"),
         f"/DReleaseDir={RELEASE_DIR.relative_to(ROOT)}",
         f"/DOutputSuffix={suffix}"],
        check=True,
    )


if __name__ == "__main__":
    main()
