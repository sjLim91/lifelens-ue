#!/usr/bin/env python3
"""Regression checks for audit scanner itself; synthetic fixtures only."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

MODULE = Path(__file__).resolve().parents[1] / "full_source_audit.py"
spec = importlib.util.spec_from_file_location("lifelens_full_source_audit", MODULE)
audit_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit_module)


class FullSourceAuditTests(unittest.TestCase):
    def test_web_import_resolution(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / "web/src/sim/core.ts"
            path.parent.mkdir(parents=True)
            path.write_text("export const x = 1;\n", encoding="utf-8")
            self.assertEqual(
                audit_module.local_typescript_target(root, Path("web/src/App.tsx"), "./sim/core"),
                Path("web/src/sim/core.ts"))
            self.assertIsNone(
                audit_module.local_typescript_target(root, Path("web/src/App.tsx"), "./sim/absent"))

    def test_findings_are_labeled_not_execution_proofs(self):
        data = {
            "Source/LifeLensCore/include/lifelens/Simulation.h":
                "// Acceptance only records an input; it does not execute or bias goals yet.\n",
            "Source/LifeLensCore/src/Simulation.cpp":
                "bool Simulation::acceptCognitiveProposal(\n",
            "Source/LifeLensCore/include/lifelens/SocialUtility.h":
                "inline SocialUtilityDecision chooseSocialUtilityDecision() {\n"
                " for (const auto& candidate : world.characters) {}\n}\n",
            "Source/LifeLensCore/src/WebClientBridge.cpp":
                'out << "\\\"snapshotPersistence\\\":false";\n',
            "Source/LifeLensCore/include/lifelens/WildlifeEcology.h":
                "// Not a pathfinder or geography-aware migration yet.\n",
        }
        findings = audit_module.findings_for(data)
        codes = {item["code"] for item in findings}
        self.assertTrue({"COG-01", "COG-02", "SOC-01", "ECO-01"}.issubset(codes))
        self.assertTrue(all(item.get("state") for item in findings))

    def test_registered_core_test_detection(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            cpp = root / "Source/LifeLensCore/tests/test_sample.cpp"
            cpp.parent.mkdir(parents=True)
            cpp.write_text("int main(){return 0;}\n", encoding="utf-8")
            cmake = root / "Source/LifeLensCore/CMakeLists.txt"
            cmake.write_text("lifelens_add_test(test_sample)\n", encoding="utf-8")
            result = audit_module.audit(root)
            self.assertFalse(any(x["code"].startswith("TEST-") for x in result["findings"]))
            self.assertGreaterEqual(result["totals"]["files"], 2)

    def test_utf8_inventory_and_output(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "example.md").write_text("한글\n", encoding="utf-8")
            result = audit_module.audit(root)
            output = root / "audit-output"
            audit_module.write_report(result, output)
            self.assertTrue((output / "source-inventory.json").is_file())
            self.assertTrue((output / "SOURCE_AUDIT.md").is_file())
            self.assertEqual(result["totals"]["files"], 1)


if __name__ == "__main__":
    unittest.main()
