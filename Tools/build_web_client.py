#!/usr/bin/env python3
from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT / "Source" / "LifeLensCore"
BUILD = ROOT / "build-web"
OUT = ROOT / "Clients" / "Web" / "generated"


def run(*args: str) -> None:
    print("+", " ".join(args), flush=True)
    subprocess.run(args, cwd=ROOT, check=True)


def main() -> None:
    if shutil.which("emcmake") is None:
        raise SystemExit(
            "emcmake not found. Activate the Emscripten SDK before building LifeLens Web."
        )

    run("emcmake", "cmake", "-S", str(CORE), "-B", str(BUILD), "-DCMAKE_BUILD_TYPE=Release")
    run("cmake", "--build", str(BUILD), "--target", "lifelens_web_core", "-j")

    built = BUILD / "web"
    js = built / "lifelens_core.js"
    wasm = built / "lifelens_core.wasm"
    if not js.exists() or not wasm.exists():
        raise SystemExit("Emscripten build completed without expected .js/.wasm outputs")

    OUT.mkdir(parents=True, exist_ok=True)
    shutil.copy2(js, OUT / js.name)
    shutil.copy2(wasm, OUT / wasm.name)
    print(f"Staged web runtime -> {OUT}")


if __name__ == "__main__":
    main()
