import os
import tempfile
import unittest

from ..passes import IfPass

class LineMarkersTestCase(unittest.TestCase):
    def setUp(self):
        self.pass_ = IfPass(external_programs = {'unifdef': 'unifdef'})

    def test_all(self):
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as tmp_file:
            tmp_file.write('#if FOO\nint a = 2;\n#endif')

        state = self.pass_.new(tmp_file.name)
        state = self.pass_.advance(tmp_file.name, state)
        (_, state) = self.pass_.transform(tmp_file.name, state)
        self.assertEqual(state.index, 0)
        self.assertEqual(state.instances, 1)

        with open(tmp_file.name, mode="r") as variant_file:
            variant = variant_file.read()

        os.unlink(tmp_file.name)
        self.assertEqual(variant, "int a = 2;\n")
