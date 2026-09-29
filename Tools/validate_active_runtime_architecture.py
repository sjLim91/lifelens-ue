#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

required = (
    root / "Source/LifeLensCore/CMakeLists.txt",
    root / "Source/LifeLensCore/wasm/LifeLensWebBindings.cpp",
    root / "Tools/build_web_client.py",
    root / "web/package.json",
    root / "docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md",
)
for path in required:
    assert path.exists(), f"required active runtime path missing: {path.relative_to(root)}"

retired = (
    root / "LifeLens.uproject",
    root / "Source/LifeLens",
    root / "Source/LifeLens.Target.cs",
    root / "Source/LifeLensEditor.Target.cs",
    root / "Config",
    root / "Content",
    root / ".github/workflows/unreal-linux-compile.yml",
    root / ".github/workflows/android-apk.yml",
)
for path in retired:
    assert not path.exists(), f"retired Unreal path returned to active main: {path.relative_to(root)}"

architecture = (root / "docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md").read_text(encoding="utf-8")
for token in (
    "LifeLensCore",
    "WASM",
    "Web Observer",
    "archive/unreal-final-20260929",
):
    assert token in architecture, f"runtime architecture missing token: {token}"

print("LifeLens active Web/Core runtime architecture: PASS")
