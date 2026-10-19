"""Provenance and dependency failures must prevent new source-byte admission."""
import importlib.util
import json
from pathlib import Path
import struct
import unittest

SPEC = importlib.util.spec_from_file_location('wildlife_sources',
    Path(__file__).resolve().parents[1] / 'research/review_distributed_wildlife.py')
sources = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(sources)


class SourcePolicy(unittest.TestCase):
    def setUp(self):
        self.original = {'name': 'Original animal', 'user': {'username': 'creator'}, 'license': 'by'}

    def test_permissive_label_does_not_override_source_restrictions(self):
        self.assertTrue(sources.eligible(self.original))
        self.assertTrue(sources.eligible({**self.original, 'license': 'cc0'}))
        for text in ['Personal use only', 'Non-commercial', 'Not my mesh', 'Ripped from Skyrim']:
            with self.subTest(text=text):
                self.assertFalse(sources.eligible({**self.original, 'description': text}))
        for license_value in ['by-nc', 'by-nd', '', {'url': 'https://example.com/unknown'}]:
            with self.subTest(license=license_value):
                self.assertFalse(sources.eligible({**self.original, 'license': license_value}))
        self.assertFalse(sources.eligible({**self.original, 'user': {}}))

    @staticmethod
    def glb(doc):
        encoded = json.dumps(doc).encode()
        encoded += b' ' * (-len(encoded) % 4)
        return struct.pack('<4sII', b'glTF', 2, 20 + len(encoded)) + struct.pack('<II', len(encoded), 0x4e4f534a) + encoded

    def test_unreviewed_external_dependencies_are_rejected(self):
        self.assertEqual(sources.inspect_glb(self.glb({'asset': {'version': '2.0'}}))['asset']['version'], '2.0')
        for doc in [{'buffers': [{'uri': 'outside.bin'}]},
                    {'images': [{'uri': 'texture.png'}]},
                    {'images': [{'uri': 'https://example.com/texture.png'}]}]:
            with self.subTest(doc=doc), self.assertRaises(ValueError):
                sources.inspect_glb(self.glb(doc))
        with self.assertRaises(ValueError):
            sources.inspect_glb(b'not a GLB')

    def test_existing_tiger_is_outside_scope(self):
        self.assertEqual(set(sources.CATEGORIES), {'Hare', 'Deer', 'Wolf', 'FreshwaterFish'})


if __name__ == '__main__':
    unittest.main()
