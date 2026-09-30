"""Publish a verified Godot export into the repository's tracked web_build directory.

All output is prepared before the previous build is replaced. Invalid/incomplete
exports leave the previous build untouched. No network or deployment is performed.
"""
import json
from pathlib import Path
import re
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def validate_export(source: Path) -> None:
    html = (source / "index.html").read_text()
    match = re.search(r"const GODOT_CONFIG\s*=\s*(\{[^\n]+\});", html)
    if not match:
        raise ValueError("The export has no Godot configuration")
    config = json.loads(match[1])
    expected = config["fileSizes"]
    for name in ("index.pck", "index.wasm"):
        if (source / name).stat().st_size != expected[name]:
            raise ValueError(f"Export is truncated or stale: {name}")
    if (source / "index.wasm").read_bytes()[:8] != b"\x00asm\x01\x00\x00\x00":
        raise ValueError("Invalid WebAssembly module")
    if (source / "index.pck").read_bytes()[:4] != b"GDPC":
        raise ValueError("Invalid Godot resource pack")
    for name in ("index.js", "index.audio.worklet.js", "index.audio.position.worklet.js",
                 "index.png", "index.icon.png", "index.apple-touch-icon.png"):
        if not (source / name).is_file() or not (source / name).stat().st_size:
            raise ValueError(f"Missing runtime asset: {name}")


def replace_build(source: Path, destination: Path) -> None:
    validate_export(source)
    staging = Path(tempfile.mkdtemp(prefix=".web-build-", dir=destination.parent))
    backup = staging / "previous-build"
    try:
        # Do not publish editor .import sidecars or caches from old export folders.
        for asset in source.iterdir():
            if asset.is_file() and asset.suffix in {".html", ".js", ".wasm", ".pck", ".png", ".svg", ".ico", ".json"}:
                shutil.copy2(asset, staging / asset.name)
        # Keep generated HTML diff-clean without changing its runtime configuration.
        page = staging / "index.html"
        page.write_text(page.read_text().rstrip() + "\n")
        notices = "\n\n".join((ROOT / "game/assets/fonts" / name).read_text()
                                for name in ("Fraunces-OFL.txt", "DMSans-OFL.txt"))
        (staging / "FONT-LICENSES.txt").write_text(notices)
        validate_export(staging)
        if destination.exists():
            destination.rename(backup)
        try:
            staging.rename(destination)
        except OSError:
            if backup.exists():
                backup.rename(destination)
            raise
        old_build = destination / "previous-build"
        if old_build.exists():
            shutil.rmtree(old_build)
    finally:
        if staging.exists():
            shutil.rmtree(staging)
    print(f"Replaced {destination.name} with verified Godot export ({sum(p.stat().st_size for p in destination.iterdir()):,} bytes)")


if __name__ == "__main__":
    replace_build(ROOT / "game/exports/web", ROOT / "web_build")
