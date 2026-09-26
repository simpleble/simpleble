#!/usr/bin/env python3
"""Package a local SimpleEmbed checkout and import an immutable SDK snapshot."""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=root.parent / "simpleembed-nrf")
    parser.add_argument("--manifest", type=Path, default=root / ".sdk/manifest.json")
    args = parser.parse_args()
    source = args.source.resolve()
    version = (source / "VERSION").read_text().strip()
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9.+_-]*", version):
        parser.error("SimpleEmbed VERSION is not a safe package name")
    subprocess.run(
        [sys.executable, str(source / ".simpleembed/scripts/package.py")],
        cwd=source,
        check=True,
    )
    package_name = f"simpleembed-nrf-{version}"
    archive = source / "dist" / f"{package_name}.zip"
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    manifest_path = args.manifest.resolve()
    cache = manifest_path.parent
    cache.mkdir(parents=True, exist_ok=True)
    destination = cache / digest
    if not destination.exists():
        with tempfile.TemporaryDirectory(dir=cache) as temporary:
            staging = Path(temporary) / digest
            staging.mkdir()
            shutil.copy2(archive, staging / archive.name)
            with zipfile.ZipFile(staging / archive.name) as package:
                for member in package.infolist():
                    target = (staging / member.filename).resolve()
                    if not target.is_relative_to(staging / package_name):
                        raise ValueError(f"Unsafe package path: {member.filename}")
                    if (member.external_attr >> 16) & 0o170000 == 0o120000:
                        raise ValueError(f"Package symlink: {member.filename}")
                package.extractall(staging)
            staging.rename(destination)
    manifest = {
        "version": version,
        "sha256": digest,
        "archive": str((destination / archive.name).relative_to(cache)),
        "root": str((destination / package_name).relative_to(cache)),
        "source_commit": subprocess.check_output(
            ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
        ).strip(),
        "source_status": subprocess.check_output(
            ["git", "-C", str(source), "status", "--short"], text=True
        ),
    }
    pending = manifest_path.with_suffix(".tmp")
    pending.write_text(json.dumps(manifest, indent=2) + "\n")
    pending.replace(manifest_path)
    print(
        f"Imported SimpleEmbed {version}\nSHA256 {digest}\n{destination / package_name}"
    )


if __name__ == "__main__":
    main()
