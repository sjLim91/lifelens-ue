# LifeLens Build Lessons

This document preserves useful lessons from the retired LOCAL OBSERVER build experiments so the old repository can be removed without losing the knowledge.

## Build strategy

- LifeLens is the only active project and repository.
- Android is the first real target; Windows comes later.
- Use only zero-cost public GitHub-hosted runners.
- Never run repeated blind multi-hour builds.
- Always inspect the exact failed step and logs before changing anything.
- Prefer short preflight checks before expensive Unreal setup/build work.
- Never claim success until the expected artifact actually exists.

## Confirmed useful results

- Unreal Engine 5.6 C++ UHT/UBT compilation was proven possible on a standard public GitHub-hosted Linux runner.
- Linux cook/package was successfully proven in prior experiments.
- Epic access through the existing GitHub/Epic credential path was confirmed.
- GitHub Actions standard runners are ephemeral: every fresh run starts clean and large Unreal dependencies/images must be fetched again unless explicitly cached.

## Android failures to never repeat

### 1. `dev-slim-5.6.0` container does not support Android target builds

Observed error:

`The Android platform is not supported from this engine distribution.`

Meaning:
- The Epic `ghcr.io/epicgames/unreal-engine:dev-slim-5.6.0` image can be useful for Linux/editor compile validation.
- It must not be reused for LifeLens Android target compilation.
- Re-running the same image for Android is wasted time.

Current direction:
- Build Android from EpicGames/UnrealEngine 5.6 source with Android platform dependencies installed.

### 2. Separate environment failures from source failures

A successful SDK/NDK or Turnkey stage does not prove the game source compiles.
A compile failure after environment setup must be treated as a separate milestone and investigated from the first real compiler error.

### 3. Do not confuse structural preflight with Unreal compile

Structural validation only proves repository/project shape and expected files/configuration.
It does not prove UHT, UBT, cook, package, install, or runtime success.

### 4. Do not run duplicate workflows

- Keep one authoritative Android pipeline in `lifelens-ue`.
- Avoid triggering Linux and Android builds from unrelated documentation commits.
- Keep expensive workflows path-filtered or manual where practical.
- Use concurrency cancellation carefully; a new run on an ephemeral runner can throw away a large download/setup investment.

## Android baseline

- Unreal Engine: 5.6
- Android API: 34
- Build Tools: 34.0.0
- NDK baseline for current source-build path: 25.1.8937393 (r25b)
- ABI: ARM64
- Package: `com.lifelens.sim`
- First goal: Development APK for real-device smoke testing

## Reporting rules

Say `compiled` only after the compile step succeeds.
Say `packaged` only after BuildCookRun succeeds.
Say `APK ready` only after an actual `.apk` file is found.
Say `download ready` only after an Actions artifact or GitHub Release exists and is verified.

## Repository rule

`sjLim91/lifelens-ue` is the single source of truth for LifeLens source, build workflows, Actions runs, artifacts, and releases.
