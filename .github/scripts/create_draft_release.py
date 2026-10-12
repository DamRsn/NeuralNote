#!/usr/bin/env python3
"""Creates the draft release a CI build uploads its installers to.

The tag is ci-<run number>. The draft is reused when it already exists, so a
re-run of failed jobs uploads to the draft of its first attempt. Writes the tag
to the step's outputs as `tag`. Needs GH_TOKEN and GH_REPO.
"""

import os
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def project_version() -> str:
    cmake = (ROOT / "CMakeLists.txt").read_text()
    match = re.search(r"^project\(NeuralNotePlugin VERSION ([0-9.]+)\)", cmake, re.MULTILINE)

    if not match:
        raise SystemExit("No project version in CMakeLists.txt")

    return match.group(1)


def main() -> None:
    env = os.environ
    run_number = env["GITHUB_RUN_NUMBER"]
    tag = f"ci-{run_number}"

    exists = subprocess.run(["gh", "release", "view", tag], capture_output=True).returncode == 0

    if exists:
        print(f"Reusing the draft release {tag}")
    else:
        run_url = f"{env['GITHUB_SERVER_URL']}/{env['GITHUB_REPOSITORY']}/actions/runs/{env['GITHUB_RUN_ID']}"
        subprocess.run(
            [
                "gh", "release", "create", tag,
                "--draft",
                "--target", env["GITHUB_SHA"],
                "--title", f"NeuralNote {project_version()} CI #{run_number}",
                "--notes", f"Built from `{env['GITHUB_REF_NAME']}` at {env['GITHUB_SHA']} by {run_url}",
            ],
            check=True,
        )

    with open(env["GITHUB_OUTPUT"], "a") as output:
        output.write(f"tag={tag}\n")


if __name__ == "__main__":
    main()
