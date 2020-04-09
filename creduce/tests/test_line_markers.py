import os
import tempfile
import unittest

from ..passes import LineMarkersPass

class LineMarkersTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = LineMarkersPass()

    def test_all(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('# 1 "foo.h"\n# 2 "bar.h"\n#4   "x.h"')

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)
        self.assertEqual(state.index, 0)
        self.assertEqual(state.instances, 3)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)
        self.assertEqual(variant, "")

    def test_only_last(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('# 1 "foo.h"\n# 2 "bar.h"\n#4   "x.h\nint x = 2;')

        state = self.pass_.new(tmp_file.name)
        (_, state) = self.pass_.transform(tmp_file.name, state)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)
        self.assertEqual(variant, "int x = 2;")
