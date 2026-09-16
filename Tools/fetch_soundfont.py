#!/usr/bin/env python3
"""Fetches the soundfont the synth plays into NeuralNote/Assets/SoundFont/.

The font is ~38 MB and is not committed. CMake runs this at configure time, so a
fresh clone still builds in one step; it is a no-op once the file is present and
its digest matches.

Run manually with --force to re-download a file that failed verification.
"""

import argparse
import hashlib
import shutil
import ssl
import sys
import urllib.request
from pathlib import Path

BASE_URL = "https://ftp.osuosl.org/pub/musescore/soundfont/MuseScore_General"

# Pinned by digest rather than by version: the distribution point serves a
# single mutable filename, and upstream has changed what it points at before (a
# _Lite variant was briefly the default MuseScore_General.sf2). A mismatch here
# means the remote file changed, not that the download broke -- re-pin
# deliberately, after re-reading the licence at the new revision.
FILES = [
    (
        "MuseScore_General.sf3",
        "5b85b6c2c61d10b2b91cddd41efcce7b25cd31c8271d511c73afafbef20b6fa3",
    ),
    # The licence requires its credits to accompany any binary embedding the
    # font; Installers/license.txt reproduces this file for that.
    (
        "MuseScore_General_License.md",
        "5ad8d737e13c7f01f5b9674872a82a92b4ba253603e8ed14b9db12293550b4b9",
    ),
]

DEST_DIR = Path(__file__).resolve().parent.parent / "NeuralNote" / "Assets" / "SoundFont"


def ssl_context() -> ssl.SSLContext:
    """A verifying context, with certifi's bundle when OpenSSL has no usable store.

    Conda-style Pythons on Windows ship an OpenSSL whose compiled-in CA path
    (C:/Program Files/Common Files/ssl) does not exist, so the default context
    verifies nothing successfully. certifi ships with those distributions.
    """
    paths = ssl.get_default_verify_paths()

    if paths.cafile or paths.capath:
        return ssl.create_default_context()

    try:
        import certifi
    except ImportError:
        return ssl.create_default_context()

    return ssl.create_default_context(cafile=certifi.where())


def sha256(path: Path) -> str:
    digest = hashlib.sha256()

    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)

    return digest.hexdigest()


def fetch(name: str, expected: str, force: bool) -> bool:
    destination = DEST_DIR / name

    if destination.exists() and not force:
        actual = sha256(destination)

        if actual == expected:
            return True

        print(f"{name}: digest mismatch, re-downloading", file=sys.stderr)

    url = f"{BASE_URL}/{name}"
    print(f"Downloading {url}")

    # To a temporary name first: an interrupted download that lands on the real
    # path would be picked up as valid-looking by the next configure.
    partial = destination.with_suffix(destination.suffix + ".partial")

    try:
        with urllib.request.urlopen(url, timeout=60, context=ssl_context()) as response, partial.open("wb") as out:
            shutil.copyfileobj(response, out)
    except Exception as error:  # noqa: BLE001 -- reported to the user, not handled
        partial.unlink(missing_ok=True)
        print(f"{name}: download failed: {error}", file=sys.stderr)
        return False

    actual = sha256(partial)

    if actual != expected:
        partial.unlink()
        print(f"{name}: expected sha256 {expected}, got {actual}", file=sys.stderr)
        return False

    partial.replace(destination)
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--force", action="store_true", help="re-download even if present")
    parser.add_argument("--print-digests", action="store_true", help="print digests of what is on disk and exit")
    args = parser.parse_args()

    DEST_DIR.mkdir(parents=True, exist_ok=True)

    if args.print_digests:
        for name, _ in FILES:
            path = DEST_DIR / name
            print(f"{sha256(path) if path.exists() else '<missing>'}  {name}")
        return 0

    return 0 if all(fetch(name, expected, args.force) for name, expected in FILES) else 1


if __name__ == "__main__":
    sys.exit(main())
