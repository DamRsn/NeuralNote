#!/usr/bin/env python3
"""Builds the Windows installer from build/NeuralNote_artefacts/Release, in two steps around code signing.

prepare: moves the PDBs out of the release directory, since the linker writes them
beside the binaries, some inside the VST3 bundle, and the installer copies those
directories whole. Then stages the binaries and the uninstaller that Inno Setup
embeds in the installer to $RUNNER_TEMP/signing/unsigned, for signing.

installer: puts the signed files from $RUNNER_TEMP/signing/signed in place and
compiles the installer.

Writes NeuralNote_Installer_Windows_x64.exe and NeuralNote_Installer_Windows_x64_PDBs.zip
to Installers/Windows/Output.
"""

import argparse
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RELEASE_DIR = ROOT / "build" / "NeuralNote_artefacts" / "Release"
OUTPUT_DIR = ROOT / "Installers" / "Windows" / "Output"
ISCC = Path(os.environ["ProgramFiles(x86)"]) / "Inno Setup 6" / "ISCC.exe"

TEMP = Path(os.environ["RUNNER_TEMP"])
SIGNING_DIR = TEMP / "signing"
UNINSTALLER_DIR = SIGNING_DIR / "uninstaller"

BINARIES = [
    Path("Standalone/NeuralNote.exe"),
    Path("VST3/NeuralNote.vst3/Contents/x86_64-win/NeuralNote.vst3"),
]


def collect_pdbs(symbols_dir: Path) -> None:
    pdbs = sorted(RELEASE_DIR.rglob("*.pdb"))

    if not pdbs:
        raise SystemExit(f"No PDBs in {RELEASE_DIR}: build Release with NEURALNOTE_SYMBOLS=ON")

    for pdb in pdbs:
        destination = symbols_dir / pdb.relative_to(RELEASE_DIR)
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(pdb, destination)
        print(f"Collected {destination.relative_to(symbols_dir)}")


def run_iscc(check: bool) -> subprocess.CompletedProcess:
    # neuralnote.iss resolves ReleaseDir from its own directory, two levels below the root.
    return subprocess.run(
        [str(ISCC), str(ROOT / "Installers" / "Windows" / "neuralnote.iss"),
         f"/DReleaseDir={RELEASE_DIR.relative_to(ROOT)}", f"/DSignedUninstallerDir={UNINSTALLER_DIR}"],
        check=check, capture_output=not check, text=True,
    )


def uninstaller() -> Path | None:
    files = list(UNINSTALLER_DIR.glob("uninst-*.e??"))
    return files[0] if len(files) == 1 else None


def prepare() -> None:
    symbols_dir = TEMP / "symbols"
    collect_pdbs(symbols_dir)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    shutil.make_archive(str(OUTPUT_DIR / "NeuralNote_Installer_Windows_x64_PDBs"), "zip", symbols_dir)

    # With no signed uninstaller in its directory, the compile writes an unsigned one there and fails.
    shutil.rmtree(UNINSTALLER_DIR, ignore_errors=True)
    UNINSTALLER_DIR.mkdir(parents=True)
    result = run_iscc(check=False)
    stub = uninstaller()

    if stub is None:
        print(result.stdout, result.stderr)
        raise SystemExit("Inno Setup did not write the uninstaller to sign")

    unsigned_dir = SIGNING_DIR / "unsigned"
    shutil.rmtree(unsigned_dir, ignore_errors=True)
    unsigned_dir.mkdir(parents=True)

    for binary in BINARIES:
        (unsigned_dir / binary).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(RELEASE_DIR / binary, unsigned_dir / binary)

    (unsigned_dir / "Uninstaller").mkdir()
    shutil.copy2(stub, unsigned_dir / "Uninstaller" / stub.name)

    for staged in sorted(unsigned_dir.rglob("*")):
        if staged.is_file():
            print(f"Staged {staged.relative_to(unsigned_dir)}")


def installer() -> None:
    signed_dir = SIGNING_DIR / "signed"
    stub = uninstaller()

    if stub is None:
        raise SystemExit(f"No uninstaller in {UNINSTALLER_DIR}: run prepare first")

    for binary in BINARIES:
        shutil.copy2(signed_dir / binary, RELEASE_DIR / binary)

    shutil.copy2(signed_dir / "Uninstaller" / stub.name, stub)
    run_iscc(check=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("step", choices=["prepare", "installer"])
    args = parser.parse_args()

    if not ISCC.exists():
        raise SystemExit(f"Inno Setup not found at {ISCC}")

    if args.step == "prepare":
        prepare()
    else:
        installer()


if __name__ == "__main__":
    main()
