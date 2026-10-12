#!/usr/bin/env python3
"""Builds the Windows installer from build/NeuralNote_artefacts/Release.

Moves the PDBs out of the release directory first, since the linker writes them
beside the binaries, some inside the VST3 bundle, and the installer copies those
directories whole. Writes NeuralNoteInstaller.exe and NeuralNoteInstaller_PDBs.zip
to Installers/Windows/Output.
"""

import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RELEASE_DIR = ROOT / "build" / "NeuralNote_artefacts" / "Release"
OUTPUT_DIR = ROOT / "Installers" / "Windows" / "Output"
ISCC = Path(os.environ["ProgramFiles(x86)"]) / "Inno Setup 6" / "ISCC.exe"


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

    symbols_dir = Path(os.environ["RUNNER_TEMP"]) / "symbols"
    collect_pdbs(symbols_dir)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    shutil.make_archive(str(OUTPUT_DIR / "NeuralNoteInstaller_PDBs"), "zip", symbols_dir)

    # neuralnote.iss resolves ReleaseDir from its own directory, two levels below the root.
    subprocess.run(
        [str(ISCC), str(ROOT / "Installers" / "Windows" / "neuralnote.iss"),
         f"/DReleaseDir={RELEASE_DIR.relative_to(ROOT)}"],
        check=True,
    )


if __name__ == "__main__":
    main()
