import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / "Tools" / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


scope = load("ci_scope")
web = load("ci_web")


class ScopeTests(unittest.TestCase):
    def test_docs_require_no_compilation_or_rendering(self):
        self.assertEqual(scope.select_scope(["docs/new.md"]),
                         {"web": False, "core": False, "long_run": False, "screens": []})

    def test_web_readout_does_not_run_core_or_365_days(self):
        result = scope.select_scope(["web/src/ui/resident-status.ts"])
        self.assertTrue(result["web"])
        self.assertFalse(result["core"])
        self.assertFalse(result["long_run"])
        self.assertEqual(result["screens"], [])

    def test_social_changes_capture_social_only(self):
        self.assertEqual(scope.select_scope(["web/src/render/social-event-layer.ts"])["screens"], ["social"])

    def test_grounding_and_dependencies_keep_cross_scene_coverage(self):
        for path in ("web/src/render/resident-grounding.ts", "web/package-lock.json",
                     "web/src/runtime/core-bridge.ts", "web/src/render/new-shared-pass.ts"):
            self.assertEqual(scope.select_scope([path])["screens"], list(scope.SCREENS), path)

    def test_core_simulation_runs_native_long_run_and_web_contract(self):
        result = scope.select_scope(["Source/LifeLensCore/src/Simulation.cpp"])
        self.assertTrue(result["core"] and result["web"] and result["long_run"])

    def test_core_tests_and_bridge_only_avoid_behavioral_long_run(self):
        for path in ("Source/LifeLensCore/tests/test_snapshot_codec.cpp",
                     "Source/LifeLensCore/src/WebClientBridge.cpp"):
            result = scope.select_scope([path])
            self.assertTrue(result["core"])
            self.assertEqual(result["web"], "/tests/" not in path)
            self.assertFalse(result["long_run"])

    def test_pure_core_ecology_contract_still_gets_environment_capture(self):
        result = scope.select_scope(["Source/LifeLensCore/include/lifelens/ContinuousTerrain.h"])
        self.assertIn("environment", result["screens"])

    def test_manual_and_nightly_request_full_verification(self):
        for event in ("schedule", "workflow_dispatch"):
            paths, full = scope.changed_paths({}, event)
            result = scope.select_scope(paths, full)
            self.assertTrue(result["web"] and result["core"] and result["long_run"])
            self.assertEqual(result["screens"], list(scope.SCREENS))

    def test_new_branch_fails_conservatively(self):
        paths, full = scope.changed_paths({"before": "0" * 40, "after": "abc"}, "push")
        self.assertTrue(full)

    def test_policy_changes_verify_routing_and_scenes_without_long_run(self):
        result = scope.select_scope([".github/workflows/web-checks.yml", "Tools/ci_scope.py"])
        self.assertTrue(result["web"])
        self.assertEqual(result["screens"], list(scope.SCREENS))
        self.assertFalse(result["long_run"])

    def test_git_diff_tracks_deletions_renames_and_literal_filenames(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            def git(*args):
                return subprocess.run(["git", *args], cwd=root, check=True,
                                      capture_output=True, text=True).stdout.strip()
            git("init", "-q")
            git("config", "user.name", "CI test")
            git("config", "user.email", "ci@example.invalid")
            old = root / "web/src/render/social-event-layer.ts"
            old.parent.mkdir(parents=True)
            old.write_text("old")
            git("add", "."); git("commit", "-qm", "base")
            base = git("rev-parse", "HEAD")
            old.unlink()
            weird = "web/src/render/with space\n$(literal).ts"
            (root / weird).write_text("old")  # Exact-content rename, not just delete + unrelated add.
            git("add", "."); git("commit", "-qm", "rename")
            head = git("rev-parse", "HEAD")
            paths, full = scope.changed_paths({"pull_request": {"base": {"sha": base}, "head": {"sha": head}}},
                                              "pull_request", root)
            self.assertFalse(full)
            self.assertIn(weird, paths)
            self.assertIn("web/src/render/social-event-layer.ts", paths)
            self.assertIn("social", scope.select_scope(paths)["screens"])
            with self.assertRaises(subprocess.CalledProcessError):
                scope.changed_paths({"before": "missing", "after": head}, "push", root)


class RunnerTests(unittest.TestCase):
    def test_entrypoints_integrations_and_future_feature_tests_run_once(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            files = ("a/run.mjs", "b/run.mjs", "b/integration.mjs", "runtime-resilience/continuity.mjs", "cognitive-agent/shadow-ui.mjs",
                     "procedural-environment/lifecycle.mjs", "procedural-environment/grounding-wildlife.mjs")
            for name in files:
                file = root / name; file.parent.mkdir(parents=True, exist_ok=True); file.touch()
            entries = web.regression_entries(root)
            self.assertEqual(len(entries), len(files))
            self.assertEqual(set(entries), {root / p for p in files})

    def test_capture_selection_deduplicates_and_accepts_absent_future_gallery(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            file = root / "procedural-environment/browser-review.mjs"
            file.parent.mkdir(); file.touch()
            self.assertEqual(web.capture_entries(root, ["environment", "environment"]), [file])
            future = root / "procedural-environment/wildlife-browser-review.mjs"; future.touch()
            self.assertEqual(set(web.capture_entries(root, ["environment"])), {file, future})
            with self.assertRaises(ValueError):
                web.capture_entries(root, ["typo"])
            with self.assertRaises(FileNotFoundError):
                web.capture_entries(root, ["social"])

    def test_legacy_automatic_workflows_are_manual_diagnostics(self):
        names = ("web-runtime-resilience", "web-typecheck", "procedural-environment-check",
                 "facility-construction-check", "settlement-trade-check", "resident-lifecycle-check",
                 "social-events-check", "production-logistics-check", "environment-consequences-check",
                 "sleep-era-check", "character-assets-check", "earth-life-navigation", "earth-solar-web",
                 "globe-region-selection", "natural-earth-globe-preview", "resident-experience-readout")
        for name in names:
            source = (ROOT / ".github/workflows" / f"{name}.yml").read_text().split("jobs:")[0]
            self.assertIn("workflow_dispatch:", source)
            self.assertNotIn("pull_request:", source)
            self.assertNotIn("  push:", source)
        common = (ROOT / ".github/workflows/web-checks.yml").read_text()
        self.assertEqual(common.count("run: npm run typecheck"), 1)
        self.assertEqual(common.count("run: npm run build"), 1)
        self.assertEqual(common.count("run: npm install --ignore-scripts --no-audit --no-fund"), 1)


if __name__ == "__main__":
    unittest.main()
