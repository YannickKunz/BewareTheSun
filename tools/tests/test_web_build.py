"""Exercise publishing success/failure against temporary build directories."""
import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("sync_web_build", ROOT / "tools/sync_web_build.py")
publisher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(publisher)


class PublishTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / "export"
        shutil.copytree(ROOT / "web_build", self.source)
        self.destination = self.root / "published"
        self.destination.mkdir()
        (self.destination / "index.html").write_text("Previous working build")
        (self.destination / "index.data").write_bytes(b"old emscripten data")

    def test_complete_export_replaces_legacy_and_skips_editor_files(self):
        (self.source / "index.png.import").write_text("editor-only")
        publisher.replace_build(self.source, self.destination)
        publisher.validate_export(self.destination)
        self.assertFalse((self.destination / "index.data").exists())
        self.assertFalse((self.destination / "index.png.import").exists())
        self.assertTrue((self.destination / "FONT-LICENSES.txt").is_file())
        self.assertEqual((self.destination / "index.pck").read_bytes(), (self.source / "index.pck").read_bytes())
        self.assertEqual(list(self.root.glob(".web-build-*")), [])

    def test_incomplete_export_leaves_previous_build_untouched(self):
        (self.source / "index.audio.worklet.js").unlink()
        with self.assertRaises(ValueError):
            publisher.replace_build(self.source, self.destination)
        self.assertEqual((self.destination / "index.html").read_text(), "Previous working build")
        self.assertEqual((self.destination / "index.data").read_bytes(), b"old emscripten data")

    def test_truncated_resource_pack_is_rejected(self):
        (self.source / "index.pck").write_bytes(b"GDPC")
        with self.assertRaises(ValueError):
            publisher.replace_build(self.source, self.destination)
        self.assertEqual((self.destination / "index.html").read_text(), "Previous working build")


if __name__ == "__main__":
    unittest.main()
