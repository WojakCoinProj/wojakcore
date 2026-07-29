#!/usr/bin/env python3
"""
Recursively copy non-system shared libraries into a macOS .app bundle and
rewrite install names to @executable_path/../Frameworks/...

macdeployqt only pulls Qt frameworks; Homebrew Boost (and friends) often leave
transitive dylibs (e.g. libboost_atomic) unresolved. This script fills those gaps.

Usage:
  python3 contrib/macdeploy/bundle-dylibs.py WojakCore-Qt.app
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

SYSTEM_PREFIXES = (
    "/System/",
    "/usr/lib/",
    "/usr/lib/system/",
    "/Library/Apple/",
)

LOADER_PATH_RE = re.compile(r"^@loader_path/(.+)$")
EXEC_PATH_RE = re.compile(r"^@executable_path/(.+)$")
RPATH_RE = re.compile(r"^@rpath/(.+)$")


def run(cmd: list[str]) -> str:
    return subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL)


def otool_deps(binary: Path) -> list[str]:
    try:
        out = run(["otool", "-L", str(binary)])
    except subprocess.CalledProcessError:
        return []
    deps: list[str] = []
    for line in out.splitlines()[1:]:
        line = line.strip()
        if not line:
            continue
        # "path (compatibility version ...)"
        path = line.split(" (", 1)[0].strip()
        deps.append(path)
    return deps


def is_system_lib(path: str) -> bool:
    if path.startswith(SYSTEM_PREFIXES):
        return True
    # Self-id lines sometimes look like absolute paths to the binary itself
    return False


def basename_lib(path: str) -> str:
    # Frameworks: Foo.framework/Versions/5/Foo -> keep framework structure via basename of last component
    return Path(path).name


def ensure_writable(path: Path) -> None:
    mode = path.stat().st_mode
    path.chmod(mode | 0o200)


def install_id(path: Path, new_id: str) -> None:
    ensure_writable(path)
    subprocess.check_call(
        ["install_name_tool", "-id", new_id, str(path)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def install_change(path: Path, old: str, new: str) -> None:
    ensure_writable(path)
    # install_name_tool returns non-zero if old is absent; ignore that
    subprocess.call(
        ["install_name_tool", "-change", old, new, str(path)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def resolve_source(dep: str, referrer: Path) -> Path | None:
    """Map a dependency install-name to a real file on disk (if we can copy it)."""
    if dep.startswith(SYSTEM_PREFIXES):
        return None

    m = LOADER_PATH_RE.match(dep)
    if m:
        candidate = (referrer.parent / m.group(1)).resolve()
        if candidate.is_file():
            return candidate
        # Also try next to already-bundled frameworks
        return None

    m = EXEC_PATH_RE.match(dep)
    if m:
        # Already bundled relative to executable — nothing to copy from outside
        return None

    m = RPATH_RE.match(dep)
    if m:
        name = m.group(1)
        # Search common Homebrew locations
        for root in ("/opt/homebrew/lib", "/usr/local/lib"):
            cand = Path(root) / name
            if cand.is_file():
                return cand
        return None

    # Absolute path
    p = Path(dep)
    if p.is_file():
        return p

    # Sometimes only basename remains after partial rewrite
    for root in ("/opt/homebrew/lib", "/usr/local/lib", "/opt/homebrew/opt"):
        # /opt/homebrew/opt/*/lib/libfoo.dylib
        root_p = Path(root)
        if not root_p.exists():
            continue
        if root == "/opt/homebrew/opt":
            matches = list(root_p.glob(f"*/lib/{Path(dep).name}"))
            if matches:
                return matches[0]
        else:
            cand = root_p / Path(dep).name
            if cand.is_file():
                return cand
    return None


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} App.app", file=sys.stderr)
        return 2

    app = Path(sys.argv[1]).resolve()
    macos = app / "Contents" / "MacOS"
    frameworks = app / "Contents" / "Frameworks"
    frameworks.mkdir(parents=True, exist_ok=True)

    exes = [p for p in macos.iterdir() if p.is_file()]
    if not exes:
        print(f"ERROR: no executables in {macos}", file=sys.stderr)
        return 1

    # Seed queue with main binaries + anything already in Frameworks
    queue: list[Path] = list(exes)
    for p in frameworks.rglob("*"):
        if p.is_file() and (
            p.suffix == ".dylib"
            or p.name.startswith("Qt")
            or "framework" in str(p).lower()
        ):
            # Prefer real Mach-O files (skip Headers, Resources text)
            try:
                kind = run(["file", "-b", str(p)])
            except subprocess.CalledProcessError:
                continue
            if "Mach-O" in kind:
                queue.append(p)

    seen_bins: set[Path] = set()
    copied = 0
    changed = 0

    while queue:
        binary = queue.pop(0)
        binary = binary.resolve()
        if binary in seen_bins:
            continue
        seen_bins.add(binary)

        deps = otool_deps(binary)
        for dep in deps:
            if is_system_lib(dep):
                continue

            # Already rewritten to our Frameworks layout
            if dep.startswith("@executable_path/../Frameworks/"):
                # Ensure the file exists; if not, try to find by basename later
                dest_name = dep.split("/")[-1]
                dest = frameworks / dest_name
                if dest.is_file():
                    continue
                # Fall through to try locate by basename
                src = resolve_source(dest_name, binary)
            elif dep.startswith("@loader_path/"):
                rel = dep.split("/", 1)[1]
                dest = frameworks / Path(rel).name
                if dest.is_file():
                    # Rewrite referrer to executable_path form for consistency
                    new = f"@executable_path/../Frameworks/{dest.name}"
                    if dep != new:
                        install_change(binary, dep, new)
                        changed += 1
                    continue
                # Locate next to referrer or in brew
                src = resolve_source(dep, binary)
                if src is None:
                    # Try brew lib by basename
                    src = resolve_source(Path(rel).name, binary)
            else:
                src = resolve_source(dep, binary)

            if src is None:
                # Skip Qt framework internal paths that macdeployqt already handled
                if ".framework/" in dep or dep.startswith("@rpath/"):
                    continue
                print(f"  warn: unresolved {dep} (from {binary.name})")
                continue

            dest_name = src.name
            dest = frameworks / dest_name

            if not dest.exists():
                print(f"  copy {src} -> Frameworks/{dest_name}")
                shutil.copy2(src, dest)
                ensure_writable(dest)
                install_id(dest, f"@executable_path/../Frameworks/{dest_name}")
                copied += 1
                queue.append(dest)

            new = f"@executable_path/../Frameworks/{dest_name}"
            if dep != new:
                install_change(binary, dep, new)
                changed += 1

        # Also rewrite this binary's own id if it lives in Frameworks
        if frameworks in binary.parents and binary.suffix == ".dylib":
            install_id(binary, f"@executable_path/../Frameworks/{binary.name}")

    # Second pass: fix @loader_path among Frameworks dylibs now that all are present
    for dylib in frameworks.glob("*.dylib"):
        for dep in otool_deps(dylib):
            m = LOADER_PATH_RE.match(dep)
            if not m:
                continue
            name = Path(m.group(1)).name
            if (frameworks / name).is_file():
                install_change(
                    dylib, dep, f"@executable_path/../Frameworks/{name}"
                )
                changed += 1

    print(f"bundle-dylibs: copied={copied} rewrites={changed} scanned={len(seen_bins)}")
    return 0


if __name__ == "__main__":
    # Fix typo in resolve_source - I used invalid syntax with colon on if
    # Actually I need to fix the RPATH_RE line - I wrote `if m:` wrong with colon on match
    sys.exit(main())
